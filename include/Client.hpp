#pragma once

#include "Webserv.hpp"

using std::string;

/**
 * A class containing client info
 * 
 * @param clienSock The socket id associated with the client
 * @param requests A FIFO array of all the requests coming from this client
 * @param responses A FIFO array of all the responses beint sent to this client
 * @param is_sending A variable to block multiple concurring recvs
 * @param is_receiving A variable to block multiple concurring sends
 * 
 */
class Client
{

	private :

	int						clientSock;
	std::deque<std::string>	requests;
	std::deque<std::string>	responses;
	bool					is_sending = false;
	bool					is_receiving = false;

	public : 

	Client();
	Client(int clientSock);

	int			&getSocket();
	std::string	&getRequest();
	std::string	&getResponse();
	void		popResponse();
	void		popRequest();
	bool		&isSending();
	bool		&isReceiving();
	void		queueResponse(std::string response);
	void		queueRequest(std::string request);

	size_t	parseRequest(char *buffer, int bytesRead);
	// void	queueResponse(string response);
};
