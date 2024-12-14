#include "../include/Webserv.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/ServerBlock.hpp"
#include "../include/cgi.hpp"


HttpRequest::HttpRequest(const std::string& request, std::vector<ServerBlock>& serverBlocks)
    : _stat_code_no(200), _filename("")
{
    // Parse the request (existing logic)
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        _stat_code_no = 400; // Bad Request
        throw std::runtime_error("Invalid HTTP request: Missing headers or body");
    }

    std::string headerPart = request.substr(0, headerEnd);
    this->_body = request.substr(headerEnd + 4);

    std::istringstream requestStream(headerPart);
    std::string requestLine;
    std::getline(requestStream, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    size_t methodEnd = requestLine.find(' ');
    if (methodEnd == std::string::npos) {
        _stat_code_no = 400;
        throw std::runtime_error("Invalid HTTP request line: Missing method");
    }
    this->_method = parseHttpMethod(requestLine.substr(0, methodEnd));

    size_t uriEnd = requestLine.find(' ', methodEnd + 1);
    if (uriEnd == std::string::npos) {
        _stat_code_no = 400;
        throw std::runtime_error("Invalid HTTP request line: Missing URI");
    }
    this->_uri = requestLine.substr(methodEnd + 1, uriEnd - methodEnd - 1);
    this->_http_version = requestLine.substr(uriEnd + 1);

    // Parse headers
    this->_headers = parseHeaders(requestStream);

    // Validate headers and determine status code
    headersGood();

    // Match the server block
    this->_request_block = matchServerBlock(serverBlocks);
}



// Destructor
HttpRequest::~HttpRequest()
{
	// Clean up resources if needed
}

ServerBlock* HttpRequest::matchServerBlock(std::vector<ServerBlock>& serverBlocks) {
	std::string host = this->getHeader("host");
	std::cout << "[DEBUG] Matching server block for host: " << host << "\n";

	for (auto& block : serverBlocks) {
		auto it = block.directive_pairs.find("server_name");
		if (it != block.directive_pairs.end() && it->second == host) {
			std::cout << "[DEBUG] Matched server block for host: " << host << "\n";
			return (&block); // Return a non-const reference
		}
	}

	throw std::runtime_error("No matching server block found for host: " + host);
}




std::string HttpRequest::parseHttpMethod(const std::string& methodStr) {
	if (methodStr == "GET") return (methodStr);
	if (methodStr == "POST") return (methodStr);
	if (methodStr == "DELETE") return (methodStr);

	std::cerr << "Unsupported HTTP method: " << methodStr << std::endl;
	throw std::runtime_error("Unsupported HTTP method: " + methodStr);

	// do a 400 BAD REQUEST ERROR
}

std::map<std::string, std::string> HttpRequest::parseHeaders(std::istringstream& requestStream)
{
	std::map<std::string, std::string> headers;
	std::string line;

	while (std::getline(requestStream, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			break;
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos)
		{
			std::string headerName = line.substr(0, colonPos);
			std::string headerValue = line.substr(colonPos + 1);

			headerName.erase(0, headerName.find_first_not_of(" \t\r\n"));
			headerName.erase(headerName.find_last_not_of(" \t\r\n") + 1);

			headerValue.erase(0, headerValue.find_first_not_of(" \t\r\n"));
			headerValue.erase(headerValue.find_last_not_of(" \t\r\n") + 1);

			std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

			headers[headerName] = headerValue;
		}
	}
	return (headers);
}


std::string HttpRequest::getHeader(const std::string& key) const
{
	std::string normalizedKey = key;
	std::transform(normalizedKey.begin(), normalizedKey.end(), normalizedKey.begin(), ::tolower);
	auto it = _headers.find(normalizedKey);
	if (it != _headers.end()) {
		return it->second;
	}
	return ""; // Return empty string if header not found
}

std::string HttpRequest::getMethod() const
{
	return _method;
}

std::string HttpRequest::getUri() const
{
	return _uri;
}

std::string HttpRequest::getHttpVersion() const
{
	return _http_version;
}

std::string HttpRequest::getBody() const
{
	return _body;
}

std::string HttpRequest::getHeaders(const std::string& key) const
{
	auto it = _headers.find(key);
	if (it != _headers.end()) {
		return it->second;
	}
	return "";
}

// Print the details of the HTTP request
void HttpRequest::debug() const
{
    std::cout << "========== HTTP Request Debug Information ==========\n";
    std::cout << "Method: " << _method << "\n";
    std::cout << "URI: " << _uri << "\n";
    std::cout << "HTTP Version: " << _http_version << "\n";
    std::cout << "Status Code: " << _stat_code_no << "\n";
    std::cout << "Filename: " << (_filename.empty() ? "None" : _filename) << "\n";
    std::cout << "Headers:\n";
    for (const auto& header : _headers) {
        std::cout << "  " << header.first << ": " << header.second << "\n";
    }
    std::cout << "Body Length: " << _body.size() << "\n";
    std::cout << "====================================================\n";
}




std::string HttpRequest::unchunkBody(const std::string& body)
{
	std::string result;
	size_t pos = 0;

	while (pos < body.size()) {
		// Find the end of the chunk size line
		size_t chunkSizeEnd = body.find("\r\n", pos);
		if (chunkSizeEnd == std::string::npos) break;

		// Parse the chunk size
		int chunkSize = std::stoi(body.substr(pos, chunkSizeEnd - pos), nullptr, 16);
		if (chunkSize == 0) break; // End of chunked body

		pos = chunkSizeEnd + 2; // Move past the chunk size and \r\n
		result += body.substr(pos, chunkSize);
		pos += chunkSize + 2; // Move past the chunk and its trailing \r\n
	}

	return result;
}
const ServerBlock& HttpRequest::getRequestBlock() const
{
	return *(_request_block);
}

int HttpRequest::getStatusCode() const
{
	return _stat_code_no;
}

void HttpRequest::setStatusCode(int statusCode)
{
	_stat_code_no = statusCode;
}

std::string HttpRequest::getFilename() const
{
	return _filename;
}

void HttpRequest::setFilename(const std::string& filename)
{
	_filename = filename;
}
void HttpRequest::validateHeaders()
{
	if (_headers.find("content-length") == _headers.end()) {
		_stat_code_no = 411; // Length Required
		throw std::runtime_error("411 Length Required: Missing Content-Length header");
	}

	if (_headers.find("content-type") == _headers.end()) {
		_stat_code_no = 400; // Bad Request
		throw std::runtime_error("400 Bad Request: Missing Content-Type header");
	}
}

void HttpRequest::parseMultipartFilename()
{
    if (_headers.find("content-type") == _headers.end() ||
        _headers["content-type"].find("multipart/form-data") == std::string::npos)
    {
        throw std::runtime_error("Content-Type is not multipart/form-data");
    }

    std::string boundary = "--" + _headers["content-type"].substr(_headers["content-type"].find("boundary=") + 9);
    boundary.erase(boundary.find_last_not_of(" \t\r\n") + 1); // Trim whitespace

    size_t boundaryPos = _body.find(boundary);
    if (boundaryPos == std::string::npos)
    {
        throw std::runtime_error("Boundary not found in multipart body");
    }

    size_t partStart = _body.find("\r\n", boundaryPos) + 2;
    while (partStart != std::string::npos && partStart < _body.size())
    {
        size_t partEnd = _body.find(boundary, partStart);
        if (partEnd == std::string::npos)
        {
            throw std::runtime_error("Missing boundary in multipart body");
        }

        size_t headerEnd = _body.find("\r\n\r\n", partStart);
        if (headerEnd == std::string::npos)
        {
            throw std::runtime_error("Invalid multipart headers");
        }

        std::string partHeaders = _body.substr(partStart, headerEnd - partStart);
        size_t filenamePos = partHeaders.find("filename=");
        if (filenamePos != std::string::npos)
        {
            size_t startQuote = partHeaders.find('"', filenamePos);
            size_t endQuote = partHeaders.find('"', startQuote + 1);
            _filename = partHeaders.substr(startQuote + 1, endQuote - startQuote - 1);
            return;
        }

        partStart = _body.find("\r\n", partEnd + boundary.size()) + 2;
    }

    throw std::runtime_error("Filename not found in multipart body");
}

void HttpRequest::headersGood()
{
    auto content_length_it = getHeaders("content-length");
    if (content_length_it.empty())
    {
        _stat_code_no = 411; // Length Required
        return;
    }

    try
    {
        size_t content_length = std::stoul(content_length_it);
        const size_t MAX_UPLOAD_SIZE = 50 * 1024 * 1024; // 50 MB
        if (content_length <= 0 || content_length > MAX_UPLOAD_SIZE)
        {
            _stat_code_no = 413; // Payload Too Large
            return;
        }
    }
    catch (const std::exception&)
    {
        _stat_code_no = 400; // Bad Request
        return;
    }

    auto content_type_it = getHeaders("content-type");
    if (content_type_it.empty())
    {
        _stat_code_no = 400; // Bad Request
        return;
    }

    // Validate content type (ensure it's multipart/form-data)
    if (content_type_it.find("multipart/form-data") != std::string::npos)
    {
        try
        {
            parseMultipartFilename();
        }
        catch (const std::runtime_error&)
        {
            _stat_code_no = 400; // Bad Request
            return;
        }
        _stat_code_no = 100; // Ready for 100 Continue
        return;
    }

    _stat_code_no = 415; // Unsupported Media Type
}
