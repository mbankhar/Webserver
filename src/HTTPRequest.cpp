#include "../include/Webserv.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/ServerBlock.hpp"
#include "../include/cgi.hpp"


// HttpRequest::HttpRequest(const std::string& request, std::vector<ServerBlock>& serverBlocks)
//     : _stat_code_no(200), _filename("")
// {
//     // Parse the request (existing logic)
//     size_t headerEnd = request.find("\r\n\r\n");
//     if (headerEnd == std::string::npos) {
//         _stat_code_no = 400; // Bad Request
//         throw std::runtime_error("Invalid HTTP request: Missing headers or body");
//     }

//     std::string headerPart = request.substr(0, headerEnd);
//     this->_body = request.substr(headerEnd + 4);

//     std::istringstream requestStream(headerPart);
//     std::string requestLine;
//     std::getline(requestStream, requestLine);
//     if (!requestLine.empty() && requestLine.back() == '\r') {
//         requestLine.pop_back();
//     }

//     size_t methodEnd = requestLine.find(' ');
//     if (methodEnd == std::string::npos) {
//         _stat_code_no = 400;
//         throw std::runtime_error("Invalid HTTP request line: Missing method");
//     }
//     this->_method = parseHttpMethod(requestLine.substr(0, methodEnd));

//     size_t uriEnd = requestLine.find(' ', methodEnd + 1);
//     if (uriEnd == std::string::npos) {
//         _stat_code_no = 400;
//         throw std::runtime_error("Invalid HTTP request line: Missing URI");
//     }
//     this->_uri = requestLine.substr(methodEnd + 1, uriEnd - methodEnd - 1);
//     this->_http_version = requestLine.substr(uriEnd + 1);

//     // Parse headers
//     this->_headers = parseHeaders(requestStream);

//     // Validate headers and determine status code
//     headersGood();

//     // Match the server block
//     this->_request_block = matchServerBlock(serverBlocks);
// }
// HttpRequest::HttpRequest(const std::string& request, std::vector<ServerBlock>& serverBlocks)
//     : _stat_code_no(200), _filename("")
// {
//     // Parse headers and validate
//     size_t headerEnd = request.find("\r\n\r\n");
//     if (headerEnd == std::string::npos) {
//         _stat_code_no = 400; // Bad Request
//         throw std::runtime_error("Invalid HTTP request: Missing headers or body");
//     }

//     // Parse headers
//     std::string headerPart = request.substr(0, headerEnd);
// 	std::istringstream headerStream(headerPart); // Convert string to istringstream
// 	_headers = parseHeaders(headerStream);      // Pass the stream to parseHeaders


//     // Validate headers
//     headersGood();

//     // Match server block
//     _request_block = matchServerBlock(serverBlocks);

//     // Don't throw an error if the body is empty at this stage
//     _body = request.substr(headerEnd + 4); // Body may or may not be present
// }

HttpRequest::HttpRequest(const std::string& request, std::vector<ServerBlock>& serverBlocks)
    : _stat_code_no(200), _filename("") {
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        _stat_code_no = 400; // Bad Request
        std::cerr << "[DEBUG] Missing headers or body in the request\n";
        throw std::runtime_error("Invalid HTTP request: Missing headers or body");
    }

    std::string headerPart = request.substr(0, headerEnd);
    _body = request.substr(headerEnd + 4); // May be empty or partial if Expect: 100-continue is used

    std::istringstream requestStream(headerPart);
    std::string requestLine;
    std::getline(requestStream, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    size_t methodEnd = requestLine.find(' ');
    if (methodEnd == std::string::npos) {
        _stat_code_no = 400;
        std::cerr << "[DEBUG] Missing method in the request line\n";
        throw std::runtime_error("Invalid HTTP request line: Missing method");
    }
    _method = parseHttpMethod(requestLine.substr(0, methodEnd));

    size_t uriEnd = requestLine.find(' ', methodEnd + 1);
    if (uriEnd == std::string::npos) {
        _stat_code_no = 400;
        std::cerr << "[DEBUG] Missing URI in the request line\n";
        throw std::runtime_error("Invalid HTTP request line: Missing URI");
    }
    _uri = requestLine.substr(methodEnd + 1, uriEnd - methodEnd - 1);
    _http_version = requestLine.substr(uriEnd + 1);

    // Parse headers
    _headers = parseHeaders(requestStream);

    // Validate headers (this may set _stat_code_no to 100 if Expect: 100-continue)
    headersGood();
	if (_stat_code_no != 200)
		return;

    // If we have an Expect: 100-continue scenario, stop here.
    // The server should respond with 100 Continue, then read the body afterward.
    if (_stat_code_no == 100) {
        std::cerr << "[DEBUG] Returning early for 100-continue. Will wait for body in a subsequent step.\n";
        return;
    }

    // If not 200 OK, something went wrong; no further processing
    if (_stat_code_no != 200) {
        std::cerr << "[DEBUG] Invalid state (status code: " << _stat_code_no << "), exiting constructor.\n";
        return;
    }

    // Match the server block
    try {
        _request_block = matchServerBlock(serverBlocks);
        std::cerr << "[DEBUG] Matched server block successfully.\n";
    } catch (const std::runtime_error& e) {
        _stat_code_no = 404; // Not Found
        std::cerr << "[DEBUG] Failed to match server block: " << e.what() << "\n";
        return;
    }

    // If the request didn't require 100-continue and we have the body now, process it.
    // If it's a normal POST (without Expect) and we got the body in `request`, parse multipart now:
    if (!_body.empty()) {
        try {
            processBody(_body);
        } catch (const std::runtime_error& e) {
            // Status code is set by processBody if needed
            std::cerr << "[DEBUG] Error processing body: " << e.what() << "\n";
        }
    }
}



void HttpRequest::processBody(const std::string& body) {
    // Check that the body length matches Content-Length
    if (body.size() != std::stoul(getHeaders("content-length"))) {
        _stat_code_no = 400; // Bad Request
        throw std::runtime_error("Body length does not match Content-Length header");
    }

    _body = body;

    // If multipart, parse the filename now since we have the full body
    if (getHeaders("content-type").find("multipart/form-data") != std::string::npos) {
        try {
            parseMultipartFilename();
        } catch (const std::runtime_error&) {
            _stat_code_no = 400; // Bad Request
            throw;
        }
    }

    // If all went well, status can remain 200 or as previously set.
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
    std::cout << "Body Length: " << _body.size() << " (Processed: " << (!_body.empty() ? "Yes" : "No") << ")\n";
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

void HttpRequest::validateHeaders() {
    if (_headers.find("content-length") == _headers.end()) {
        _stat_code_no = 411; // Length Required
        std::cerr << "[DEBUG] Missing Content-Length header\n";
        throw std::runtime_error("411 Length Required: Missing Content-Length header");
    }

    if (_headers.find("content-type") == _headers.end()) {
        _stat_code_no = 400; // Bad Request
        std::cerr << "[DEBUG] Missing Content-Type header\n";
        throw std::runtime_error("400 Bad Request: Missing Content-Type header");
    }
}

void HttpRequest::headersGood() {
    // Check Content-Length
    auto content_length_str = getHeaders("content-length");
    if (content_length_str.empty()) {
        _stat_code_no = 411; // Length Required
        return;
    }

    try {
        size_t content_length = std::stoul(content_length_str);
        const size_t MAX_UPLOAD_SIZE = 50 * 1024 * 1024; // 50 MB
        if (content_length <= 0 || content_length > MAX_UPLOAD_SIZE) {
            _stat_code_no = 413; // Payload Too Large
            return;
        }
    } catch (const std::exception&) {
        _stat_code_no = 400; // Bad Request
        return;
    }

    // Check Expect: 100-continue BEFORE attempting to parse multipart body
    auto expect_header = getHeaders("expect");
    if (!expect_header.empty() && expect_header == "100-continue") {
        std::cerr << "[DEBUG] Expect: 100-continue header detected, setting status code to 100\n";
        _stat_code_no = 100; 
        return; // Return now, so we can respond with 100 Continue.
    }

    // Check Content-Type
    auto content_type_str = getHeaders("content-type");
    if (content_type_str.empty()) {
        _stat_code_no = 400; // Bad Request (Body is expected but Content-Type missing)
        return;
    }

    // If we get here without returning, we have a valid header set and no `Expect: 100-continue`
    // The body (if any) should be already present in _body if it's a simple request (like a POST without Expect).
    // We'll parse multipart, if needed, later in processBody().
    _stat_code_no = 200; // OK
}
