// #pragma once

// #include "Webserv.hpp"
// #include "ServerBlock.hpp"
// #include "Config.hpp"
// #include <string>
// #include <map>


// // Class to represent an HTTP Request
// /**
//  * HttpRequest:
//  * A class to store a parsed http request before its processed
//  * 
//  * @param method The method being requested
//  * @param uri The requested uri
//  * @param httpVersion The http version
//  * @param headers A map containing all of the headers of the request
//  * @param body The body of the request
//  * @param rootDir ?
//  * @param filePath ?
//  */
// class HttpRequest 
// {

// private:
// 	std::string							_method;                           // HTTP method (e.g., GET, POST)
// 	std::string							_uri;                             // Requested URI
// 	std::string							_http_version;                     // HTTP version (e.g., HTTP/1.1)
// 	std::map<std::string, std::string>	_headers;						  // Headers as key-value pairs
// 	std::string							_body;                            // Request body (e.g., for POST)
// 	ServerBlock*						_request_block;
// 	int                                 _stat_code_no;						//TODO:
//     std::string                         _filename;							//TODO:


// public:

// 	HttpRequest(const std::string& request, std::vector<ServerBlock>& server_blocks);
// 	~HttpRequest();

// 	std::string getHeader(const std::string& key) const;
// 	std::string getMethod() const;
// 	std::string getUri() const;
// 	std::string getHttpVersion() const;
// 	std::string	getBody() const;
	
// 	std::string							getHeaders(const std::string& headerName) const;
// 	std::map<std::string, std::string>	parseHeaders(std::istringstream& requestStream);
// 	std::string							unchunkBody(const std::string& body);
// 	std::string							parseHttpMethod(const std::string& methodStr);
// 	const ServerBlock&					getRequestBlock() const;

// 	ServerBlock*						matchServerBlock(std::vector<ServerBlock>& serverBlocks);
// 	void								debug() const;
// };

#pragma once

#include "Webserv.hpp"
#include "ServerBlock.hpp"
#include "Config.hpp"
#include <string>
#include <map>

// Class to represent an HTTP Request
/**
 * HttpRequest:
 * A class to store a parsed http request before it's processed
 * 
 * @param method The method being requested
 * @param uri The requested URI
 * @param httpVersion The HTTP version
 * @param headers A map containing all of the headers of the request
 * @param body The body of the request
 */
class HttpRequest 
{

private:
	std::string							_method;                           // HTTP method (e.g., GET, POST)
	std::string							_uri;                              // Requested URI
	std::string							_http_version;                     // HTTP version (e.g., HTTP/1.1)
	std::map<std::string, std::string>	_headers;						  // Headers as key-value pairs
	std::string							_body;                             // Request body (e.g., for POST)
	ServerBlock*						_request_block;
	int                                 _stat_code_no;						// Status code for the request
    std::string                         _filename;							// Filename to store uploaded file

public:

	HttpRequest(const std::string& request, std::vector<ServerBlock>& server_blocks);
	~HttpRequest();

	// Getters
	std::string getHeader(const std::string& key) const;
	std::string getMethod() const;
	std::string getUri() const;
	std::string getHttpVersion() const;
	std::string	getBody() const;
	int			getStatusCode() const;
	std::string	getFilename() const;

	// Setters
	void		setStatusCode(int statusCode);
	void		setFilename(const std::string& filename);

	// Parsing and processing
	std::string							getHeaders(const std::string& headerName) const;
	std::map<std::string, std::string>	parseHeaders(std::istringstream& requestStream);
	std::string							unchunkBody(const std::string& body);
	std::string							parseHttpMethod(const std::string& methodStr);
	const ServerBlock&					getRequestBlock() const;

	ServerBlock*						matchServerBlock(std::vector<ServerBlock>& serverBlocks);
	void								debug() const;

	// New methods
	void validateHeaders();               // Validates headers for errors, sets status code
	void parseMultipartFilename();        // Parses multipart body to extract filename
	void headersGood();


};

