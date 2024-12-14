#include "../include/Server.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/Client.hpp"
#include "../include/Config.hpp"
#include "../include/Webserv.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

class log;

extern std::atomic<bool> keepRunning;

Server::Server(std::vector<ServerBlock>& server_blocks, int i) : _server_blocks(server_blocks)
{
	int port = std::stoi(server_blocks[i].directive_pairs["listen"]);// TODO: need to make this cycle through all server blocks
	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock < 0) throw std::runtime_error("Socket creation failed");

	int flags = fcntl(serverSock, F_GETFL, 0);
	if (flags < 0 || fcntl(serverSock, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("Failed to set non-blocking mode");

	int opt = 1;
	if (setsockopt(this->serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("Failed to set non-blocking mode");

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
		throw std::runtime_error("Bind failed");

	if (listen(serverSock, 5) < 0)
		throw std::runtime_error("Listen failed");

	kq = kqueue();
	if (kq < 0) throw std::runtime_error("kqueue creation failed");

	struct kevent event;
	EV_SET(&event, serverSock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
	if (kevent(kq, &event, 1, nullptr, 0, nullptr) < 0)
		throw std::runtime_error("Failed to add server socket to kqueue");
}

Server::~Server()
{
	close(serverSock);
	close(kq);
	std::cout << "Server Destroyed\n";
}

/** 
 * This is the main server loop, using events to hande different situations and redirecting the program to the correct funtions
*/
void Server::run()
{
	while (keepRunning)
	{
		struct kevent eventList[1024];
		int eventCount = kevent(kq, nullptr, 0, eventList, 1024, nullptr);

		if (eventCount < 0) {
			if (errno == EINTR) continue;
			throw std::runtime_error("kevent() failed");
		}

		for (int i = 0; i < eventCount; ++i) 
		{
			int event = eventList[i].ident;
			if (eventList[i].filter == EVFILT_READ)
			{
				if (event == serverSock)
					acceptClient();
				else
				{
					msg_receive(this->clients.at(event), 0);
				}
			}
			else if (eventList[i].filter == EVFILT_USER)
			{
				if (event % 10 == 0)
					msg_receive(this->clients[event/10], 1);
				else if (event % 10 == 1)
				{
					HttpRequest		request(this->clients[event/10].getRequest(), this->_server_blocks);
					request.debug();
					if (request.getMethod() == "POST")
					{
						handlePost(event/10, this->clients[event/10].getRequest());
						this->clients[event/10].popRequest();
						cout << "HANDLE POST CALLED\n";
					}
					else
					{
						HttpResponse	response(request);
						this->clients[event/10].popRequest();
						this->clients[event/10].queueResponse(response.returnResponse());
						this->postEvent(event/10, 2);
						response.debug();
					}
				}
				else if (event % 10 == 2)
				{
					msg_send(this->clients[event/10], 0);
					cout << "SEND CALLED\n";
				}
				this->removeEvent(event);
			}
			else if (eventList[i].filter == EVFILT_WRITE)
			{
				msg_send(this->clients[event], 1);  
			}
		}
	}
}

void Server::acceptClient() {
    while (true) {
        int clientSock = accept(serverSock, nullptr, nullptr);
        if (clientSock < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
                break; // No more connections to accept
            throw std::runtime_error("Failed to accept new client");
        }

        // Set client socket to non-blocking mode
        int flags = fcntl(clientSock, F_GETFL, 0);
        if (flags == -1 || fcntl(clientSock, F_SETFL, flags | O_NONBLOCK) == -1) {
            close(clientSock);
            throw std::runtime_error("Failed to set client socket to non-blocking mode");
        }

        // Register the client socket with kqueue
        struct kevent event;
        EV_SET(&event, clientSock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
        if (kevent(kq, &event, 1, nullptr, 0, nullptr) < 0) {
            close(clientSock);
            throw std::runtime_error("Failed to add client socket to kqueue");
        }

        this->clients[clientSock] = Client(clientSock);
    }
}


std::string Server::readFile(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("File not found");

	std::ostringstream content;
	content << file.rdbuf();
	return content.str();
}

std::string Server::getMimeType(const std::string& filePath)
{
	size_t dotPos = filePath.find_last_of('.');
	if (dotPos == std::string::npos) return "application/octet-stream";

	std::string extension = filePath.substr(dotPos + 1);

	if (extension == "html" || extension == "htm") return "text/html";
	if (extension == "css") return "text/css";
	if (extension == "js") return "application/javascript";
	if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
	if (extension == "png") return "image/png";
	if (extension == "gif") return "image/gif";
	if (extension == "txt") return "text/plain";

	return "application/octet-stream";
}

void	Server::handlePost(int clientSock, string &httpRequest)
{
	this->clients[clientSock].queueRequest(httpRequest.substr(httpRequest.find("U"), httpRequest.size()));
	this->clients[clientSock].queueResponse("HTTP/1.1 100 Continue/r/n/r/n");
	this->postEvent(clientSock, 2);
}
