#include "../include/Webserv.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/ServerBlock.hpp"

#include <cstdio>

class HttpRequest;

HttpResponse::HttpResponse(const HttpRequest& request)
{
	this->_http_version = request.getHttpVersion();
	this->_chunking_required = false;
	this->_stat_code_no = request.getStatusCode();
	if (this->_stat_code_no != 200)
	{
		setErrorFilePath(request);
		setStatusCode(this->_stat_code_no, request);
		setBody(true, request);
		setHeaders(request);
	}
}

HttpResponse::~HttpResponse()
{
	
}

void HttpResponse::setFilePath(const HttpRequest& request)
{
	// TODO: make sure it is checking all subfolders and not just e.g. if you have /test/folder/uploads/ rather than /uploads/

	const ServerBlock& block = request.getRequestBlock();
	const std::map<std::string, std::map<std::string, std::string>>& locations = block.location_blocks;
	std::string matchedLocation = "/";
	size_t longestMatch = 0;
	for (const auto& location : locations)
	{
		if (request.getUri().find(location.first) == 0 && location.first.size() > longestMatch) {
			matchedLocation = location.first;
			longestMatch = location.first.size();
		}
	}
	if (locations.find(matchedLocation) == locations.end())
	{
		this->_stat_code_no = 404;
		return;
	}
	const auto& locationConfig = locations.at(matchedLocation);
	try {
		this->_file_path = resolvePath(request.getUri(), block, locationConfig);
		if (locationConfig.find("allow_methods") != locationConfig.end())
		{
			std::set<std::string> allowedMethods;
			std::istringstream methods(locationConfig.at("allow_methods"));
			std::string method;
			while (methods >> method)
				allowedMethods.insert(method);
			if (allowedMethods.find(request.getMethod()) == allowedMethods.end())
			{
				this->_stat_code_no = 405;
				return;
			}
		}
		std::ifstream file(this->_file_path);
		if (!file.good())
		{
			this->_stat_code_no = 404;
			return;
		}
		file.close();
	}
	catch (const std::runtime_error& e)// TODO: is this the correct way to use try catch ?????????????????????
	{
		std::cerr << e.what() << "\n";
		{
			this->_stat_code_no = 400;
			return;
		}
	}
	return;
}


std::string HttpResponse::resolvePath(const std::string& uri, const ServerBlock& block, const std::map<std::string, std::string>& locationConfig)
{
    std::string rootDir;
    if (locationConfig.find("root") != locationConfig.end())
        rootDir = locationConfig.at("root");
    else if (block.directive_pairs.find("root") != block.directive_pairs.end())
        rootDir = block.directive_pairs.at("root");
    else
        throw std::runtime_error("[ERROR] No root defined for the request.");
    std::string strippedUri = uri;
    if (locationConfig.find("prefix") != locationConfig.end())
    {
        std::string prefix = locationConfig.at("prefix");
        if (uri.find(prefix) == 0)
        {
            strippedUri = uri.substr(prefix.length());
        }
    }
    std::string path = rootDir + strippedUri;
	std::cerr << "[DEBUG] Resolving path for URI '" << uri << "'\n";
	std::cerr << "[DEBUG] Resolved path: " << path << "\n";

    if (path.find("..") != std::string::npos)
    {
        throw std::runtime_error("[ERROR] Invalid path: Directory traversal attempt");
    }
    std::cerr << "[DEBUG] Resolved path for URI " << uri << " | " << path << "\n";
    return path;
}

std::string HttpResponse::getFromLocation(const std::string& location, const std::string& key, const HttpRequest& request) {
    const ServerBlock& block = request.getRequestBlock();

    // Check for location-specific configuration
    if (!location.empty() && block.location_blocks.find(location) != block.location_blocks.end()) {
        const auto& locationConfig = block.location_blocks.at(location);

        // Check for location-specific error_page_<error_code>
        if (locationConfig.find("error_page_" + key) != locationConfig.end()) {
            std::cerr << "[DEBUG] Found location-specific error_page for '" << key
                      << "' in location '" << location << "' with value '"
                      << locationConfig.at("error_page_" + key) << "'\n";
            return locationConfig.at("error_page_" + key);
        }

        // Check for other location-specific directives
        if (locationConfig.find(key) != locationConfig.end()) {
            std::cerr << "[DEBUG] Found key '" << key << "' in location '" << location
                      << "' with value '" << locationConfig.at(key) << "'\n";
            return locationConfig.at(key);
        }
    }

    // Check for global directives
    if (block.directive_pairs.find(key) != block.directive_pairs.end()) {
        return block.directive_pairs.at(key);
    }

    // Check for global error_pages
    if (block.error_pages.find(key) != block.error_pages.end()) {
        return block.error_pages.at(key);
    }

    throw std::runtime_error("Data '" + key + "' not found in the location '" + location + "' or globally.");
}

void HttpResponse::setErrorFilePath(const HttpRequest& request)
{
	std::string error_code_str = std::to_string(this->_stat_code_no);

	try {
		// Use get_from_location to fetch the error page configuration
		std::string errorPagePath = getFromLocation(this->_file_path, "error_page", request);

		// Construct the full path to the error page
		this->_file_path = resolvePath(errorPagePath + "/" + error_code_str + ".html", request.getRequestBlock(), {});
	} catch (const std::runtime_error& e) {
		// Log the error for debugging purposes
		std::cerr << "[ERROR] Error while setting error file path: " << e.what() << "\n";

	}
}





void	HttpResponse::setStatusCode(const int status_code_no, const HttpRequest& request)
{
	this->_stat_code_no = status_code_no;
	auto it = _error_status_codes.find(this->_stat_code_no);
	if (it != _error_status_codes.end())
		this->_status_code = it->second;
	else
		std::cout << "UNKNOWN STATUS CODE\n";// TODO: need to decide what to do in this situation!!!!!!!!!!!!!!!!!
	if (this->_stat_code_no != 200 && this->_stat_code_no != 201)
		setErrorFilePath(request);// TODO: do this -----------------------------------
}

void	HttpResponse::setBody(bool is_first_try, const HttpRequest& request)
{
	std::stringstream	buffer;
	std::ifstream		file(this->_file_path, std::ios::binary);

	if (file.is_open())
	{
		buffer << file.rdbuf();
		std::string file_contents = buffer.str();
		file.close();
		this->_body = file_contents;
	}
	else if (is_first_try)
	{
		setStatusCode(404, request);
		setBody(false, request);
	}
	else
		this->_body = "<HTML>"
		"<H1>404 Not Found</H1>"
		"<P>The requested resource was not found.</P>"
		"</HTML>";
}

void	HttpResponse::setHeaders(const HttpRequest& request)
{
	if (this->_stat_code_no == 100)
		return;
	this->_headers["Server"] = "webserv/42.0";
	this->_headers["Date"] = this->setDateHeader();
	this->_headers["Content-Type"] = "text/html; charset=UTF-8";
	this->_headers["Connection"] = "close";
	if (this->_stat_code_no == 405 || this->_stat_code_no == 501)
		this->_headers["allowed"] = "GET POST DELETE";// TODO: need to get this from the config file of the server

	if (request.getMethod() == "????")
		return;// TODO: need to decide if we really need request here ----------------------------------


	// if (status_code_no == 200 && request.getMethod() == "GET")
	// {
	// 	this->_headers["Last-Modified"] = this->setLastModifiedHeader();
	// 	this->_headers["Content-Type"] = this->setMimeTypeHeader();
	// }
	// else if (status_code_no == 201 && request.getMethod() == "POST")
	// {

	// }
	// else if (status_code_no == 200 && request.getMethod() == "DELETE")
	// {

	// }
	// else
	// {
	// 	this->_headers["Content-Type"] = "text/html; charset=UTF-8";
	// 	this->_headers["Connection"] = "close";
	// 	if (status_code_no == 405 || status_code_no == 501)
	// }
}

std::string HttpResponse::getHeaderList()
{
	std::string		headers_list;

	for (const auto& pair : this->_headers)
	{
		// std::cout << "Key: -->" << pair.first << "<-- Value: -->" << pair.second << "<--\n";
		headers_list += pair.first + ": " + pair.second + "\r\n";
		if (pair.first == "Transfer-Encoding")
			this->_chunking_required = true;
	}
	if (!_chunking_required)
		headers_list += "Content-Length: " + std::to_string(_body.length()) + "\r\n\r\n";// TODO: this needs to be looked at

	return (headers_list);
}
std::string HttpResponse::getFilePath()
{
	return (this->_file_path);
}

std::string	HttpResponse::returnResponse()
{
	std::string response;

	response =	this->_http_version + " " + _status_code + "\r\n" +
				this->getHeaderList() + "\r\n" +
				this->_body;

	return (response);
}

std::string	HttpResponse::makeTimestampStr(std::tm* time)
{
	std::ostringstream timestamp_stream;

	timestamp_stream << std::put_time(time, "%a") << ", "
					<< std::put_time(time, "%d %b %Y ")
					<< std::put_time(time, "%X %Z");

	return (timestamp_stream.str());
}

std::string	HttpResponse::setDateHeader()
{
	std::time_t time_since_epoch = std::time(nullptr);
	std::tm* current_date_obj = std::localtime(&time_since_epoch);
	
	std::string current_date_str = makeTimestampStr(current_date_obj);

	return (current_date_str);
}

std::string	HttpResponse::setLastModifiedHeader()
{
	// std::filesystem::file_time_type lw_time = std::filesystem::last_write_time(this->_file_path);

	// std::time_t sctp = decltype(lw_time)::clock::to_time_t(lw_time);
	// std::tm* last_modified_obj = std::localtime(&sctp);
	
	// std::string last_modified_str = makeTimestampStr(last_modified_obj);
	std::string last_modified_str = "";

	return (last_modified_str);
}

std::string HttpResponse::setMimeTypeHeader()
{
	size_t dotPos = this->_file_path.find_last_of('.');

	if (dotPos == std::string::npos)
		return "application/octet-stream";

	std::string extension = this->_file_path.substr(dotPos + 1);

	if (extension == "html" || extension == "htm")
		return "text/html";
	if (extension == "css")
		return "text/css";
	if (extension == "js")
		return "application/javascript";
	if (extension == "jpg" || extension == "jpeg")
		return "image/jpeg";
	if (extension == "png")
		return "image/png";
	if (extension == "gif")
		return "image/gif";
	if (extension == "txt")
		return "text/plain";

	return "application/octet-stream";
}

void	HttpResponse::debug()
{
	std::cout << "\n =============== START LINE ===============\n\n";
	std::cout << "\n\n" << _http_version << " " << _status_code << "\n\n";

	std::cout << "\n =============== HEADERS ===============\n\n";
	std::cout << "\n\n" << getHeaderList() << "\n\n";

	std::cout << "\n =============== BODY ===============\n\n";
	std::cout << "\n\n" << _body << "\n\n";
	
	std::cout << "\n\nfile_path: " << this->_file_path << "\n";

	std::cout << "\n -----------------------------------------------------------\n\n";

}
