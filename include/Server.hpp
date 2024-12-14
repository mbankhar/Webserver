#pragma once

#include "Webserv.hpp" 
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "Client.hpp"
#include "ServerBlock.hpp"
#include <vector>
#include <map>
#include <set>

class HttpRequest; // Forward declaration
class HttpResponse; // Forward declaration
class Client;

using std::string;

/**
 * A Class representing the server, used to create a socket and listen to inncoming connections or requests.
 * 
 * @param serverSock An Integer containing the file descriptor for the server socket.
 * @param kq An integer representing the KQueue.
 * @param clients A map containing all the clients, indexedy their socket fd.
 * @param serverBlock The config block for this server
 */
class Server {
public:
	// Server();
	~Server();
	Server(std::vector<ServerBlock>& server_blocks, int i);
	void run();
	void handleGet(int clientSock, HttpRequest& httpRequest);
	void handlePost(int clientSock, string& httpRequest);
	void handleDelete(int clientSock, HttpRequest& httpRequest);
	// std::string resolvePath(const std::string& uri);

private:
	std::vector<ServerBlock> 	_server_blocks;
	int			 				serverSock;
	int 						kq;
	std::map<int, Client>		clients;

	void acceptClient();
	// void sendResponse(int clientSock, const std::string& body, int statusCode, const std::string& contentType = "text/plain");

	std::string readFile(const std::string& filePath); // Function to read static files
	std::string getMimeType(const std::string& filePath);

	void	enable_write_listen(int clientSock);
	void	disable_write_listen(int clientSock);
	void	msg_send(Client &client, int mode);
	void	msg_receive(Client &client, int mode);

	//Posting events
	void	postEvent(int clientSock, int mode);
	void	removeEvent(int eventID);
};
