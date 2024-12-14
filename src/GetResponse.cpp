#include "../include/Webserv.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/GetResponse.hpp"

GetResponse::GetResponse(HttpRequest& request) : HttpResponse(request)
{
	if (this->_stat_code_no != 200)
		return;
	setFilePath(request);
	setStatusCode(this->_stat_code_no, request);
	setBody(true, request);
	setHeaders(request);
}

GetResponse::~GetResponse()
{
	
}

void GetResponse::setHeaders(const HttpRequest& request)
{
	this->_headers["Server"] = "webserv/42.0";
	this->_headers["Date"] = this->setDateHeader();
	this->_headers["Content-Type"] = "text/html; charset=UTF-8";

	if (this->_stat_code_no == 200 || this->_stat_code_no == 201)
	{
		this->_headers["connection"] = request.getHeaders("connection");
		this->_headers["Last-Modified"] = this->setLastModifiedHeader();
		this->_headers["Content-Type"] = this->setMimeTypeHeader();
	}
	else
	{
		this->_headers["Connection"] = "close";
		if (this->_stat_code_no == 405 || this->_stat_code_no == 501)
			this->_headers["allowed"] = "GET PUT DELETE";// TODO: need to get this from the config file of the server
	}
}
