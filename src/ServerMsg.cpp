/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerMsg.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rchavez@student.42heilbronn.de <rchavez    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/06 12:37:14 by rchavez           #+#    #+#             */
/*   Updated: 2024/12/14 19:24:58 by rchavez@stu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include <errno.h>

/**
 * Adds a write event to the kqueue
 */
void	Server::enable_write_listen(int	clientSock)
{
	struct kevent enable;
	EV_SET(&enable, clientSock, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
	if (kevent(this->kq, &enable, 1, nullptr, 0, nullptr) < 0)
	{
		close(clientSock);
		this->clients.erase(clientSock);
		throw std::runtime_error("Failed to listen for write readiness");
	}
}

/**
 * Removes the write event form the kqueue
 */
void	Server::disable_write_listen(int	clientSock)
{
	struct kevent disable;
	EV_SET(&disable, clientSock, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
	if (kevent(this->kq, &disable, 1, nullptr, 0, nullptr) < 0)
	{
		close(clientSock);
		this->clients.erase(clientSock);
		throw std::runtime_error("Failed disable listen for write readiness");
	}
}
/**
 * Posts custom events,
 * 
 * 0 for read
 * 1 for process
 * 2 for write
 */
void	Server::postEvent(int clientSock, int mode)
{
	struct kevent	event;
	int				ident;

	ident = (clientSock * 10) + mode;
	EV_SET(&event, ident, EVFILT_USER, EV_ADD | EV_ENABLE, NOTE_TRIGGER, 0, nullptr);
	if (kevent(kq, &event, 1, nullptr, 0, nullptr) < 0) {
		throw std::runtime_error("Failed to post custom event to kqueue");
	}

}

void	Server::removeEvent(int eventID)
{
	struct kevent	event;

	EV_SET(&event, eventID, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
	if (kevent(kq, &event, 1, nullptr, 0, nullptr) < 0)
		throw std::runtime_error("Failed to remove custom event from kqueue: " + std::string(strerror(errno)));
}


/**
 * Send message, post event 2 to postpone a send until the previous is finished
 * Posts Write listen to keep sending when not finished
 */
void		Server::msg_send(Client &client, int mode)
{
	size_t	bytes;

	if (!mode && client.isReceiving())
	{
		this->postEvent(client.getSocket(), 2);
		return;
	}

	string	*msg =&client.getResponse();
	while (!msg->empty())
	{
		bytes = send(client.getSocket(), msg->data(), msg->size(), 0);
		if	(bytes > 0)
		{
			msg->erase(0, bytes);
			if (msg->empty())
			{
				client.popResponse();
				if (mode)
				{
					disable_write_listen(client.getSocket());
					client.isReceiving() = false;
				}
			}
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			if (!mode)
			{
				client.isReceiving() = true;
				enable_write_listen(client.getSocket());
			}
			break;
		}
		else
		{
			if (mode)
				disable_write_listen(client.getSocket());
			close(client.getSocket());
			this->clients.erase(client.getSocket());
			throw std::runtime_error("Failed to send to client socket " + std::to_string(client.getSocket()) + ": " + strerror(errno));
		}
	}
}

/**
 * Receiving message, posts event 0 to continue reading
 * Post event 1 n times to process n requests
 */
void	Server::msg_receive(Client &client, int mode)
{
	const size_t bufferSize = 4096;
	char	buffer[bufferSize];
	int	bytes;
	size_t	received;

	cout << "RECEIVE CALLED\n";
	bytes = recv(client.getSocket(), buffer, bufferSize, 0);
	cout << "Received bytes: " << bytes << "\n";
	if (bytes > 0)
	{
		received = client.parseRequest(&buffer[0], bytes);
		for (size_t i = 0; i < received; i++)
			this->postEvent(client.getSocket(), 1);
		if (bytes == bufferSize)
			this->postEvent(client.getSocket(), 0);
		else
		{
			client.getRequest();
		}
	}
	else if (bytes == 0 && !mode)
	{
		close(client.getSocket());
		this->clients.erase(client.getSocket());
	}
	else
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;//Not sure
		close(client.getSocket());
    	this->clients.erase(client.getSocket());
    	throw std::runtime_error("Failed to receive from client socket " + std::to_string(client.getSocket()) + ": " + strerror(errno));
	}
}