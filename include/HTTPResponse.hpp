#pragma once

#include "Webserv.hpp"
#include "HTTPRequest.hpp"
#include "ServerBlock.hpp"

#include <fstream>
#include <sstream>

/**
 * HttpResponse
 * 
 * A class to  store a response before its ready to send
 * 
 * @param _http_version The version number
 * @param _status_code The retunr code for the response
 * @param _headers A map of all the headers contained in the response
 * @param _chunking_required ?
 * @param _body The body of the response
 * @param _file_path ?
 */

class HttpResponse
{
protected:
	std::string							_http_version;
	int									_stat_code_no;
	std::string							_status_code;
	std::map<std::string, std::string>	_headers;
	bool								_chunking_required;
	std::string							_body;
	std::string							_file_path;
    std::string 						_matched_location;

	const std::map<int, std::string>	_error_status_codes = {
	{100, "100 Continue"}, {200, "200 OK"}, {201, "201 Created"}, {400, "400 Bad Request"}, 
	{401, "401 Unauthorized"}, {403, "403 Forbidden"}, {404, "404 Not Found"}, 
	{405, "405 Method Not Allowed"}, {411, "411 Length Required"}, {413, "413 Payload Too Large"}, 
	{415, "415 Unsupported Media Type"}, {500, "500 Internal Server Error"}, 
	{501, "501 Not Implemented"}, {502, "502 Bad Gateway"}, 
	{503, "503 Service Unavailable"}, {504, "504 Gateway Timeout"}};

public:
	HttpResponse();
	HttpResponse(const HttpRequest& request);
	virtual ~HttpResponse();

	void 			setFilePath(const HttpRequest& request);
	std::string		resolvePath(const std::string& uri, const ServerBlock& block, const std::map<std::string, std::string>& locationConfig);
	void			setStatusCode(const int status_code_no, const HttpRequest& request);
	void			setBody(bool is_first_try, const HttpRequest& request);
	virtual void	setHeaders(const HttpRequest& request);
	void			setErrorFilePath(const HttpRequest& request);

	std::string		makeTimestampStr(std::tm* time);
	std::string		setDateHeader();
	std::string		setLastModifiedHeader();
	std::string		setMimeTypeHeader();
	std::string		getHeaderList();
	std::string		getFilePath();
	std::string		returnResponse();
	std::string		getFromLocation(const std::string& location, const std::string& data, const HttpRequest& request);
	void			debug();
};
