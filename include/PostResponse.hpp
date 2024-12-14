#pragma once

#include "Webserv.hpp"
#include "HTTPResponse.hpp"
#include "HTTPRequest.hpp"
#include "ServerBlock.hpp"

#include <fstream>
#include <sstream>

/**
 * PostResponse
 * 
 * A class to  store a response before its ready to send
 * 
 * @param _filename Name of the file to be deleted
 */

class PostResponse : public HttpResponse
{
private:
	std::string							_filename;


public:
	PostResponse();
	PostResponse(HttpRequest& request);
	~PostResponse() override;

	void		setHeaders(const HttpRequest& request) override;

	// int		handlePost(const HttpRequest& httpRequest);
	void		saveFile(const HttpRequest& request);
	void		postForm(const HttpRequest& request);
};
