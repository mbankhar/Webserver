#pragma once

#include "Webserv.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

/**
 * GetResponse
 * 
 * A class for ? (
 * Seriously guys what ara all this classes for)
 * 
 */
class GetResponse : public HttpResponse
{
public:
	GetResponse();
	GetResponse(HttpRequest& request);
	~GetResponse() override;

	void		setHeaders(const HttpRequest& request) override;
	// void		parseBody();
};
