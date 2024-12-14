#include "../include/Webserv.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/PostResponse.hpp"

PostResponse::PostResponse(HttpRequest& request) : HttpResponse(request)
{
	if (this->_stat_code_no != 200)
		return;
	if (request.getFilename() == "")
		postForm(request);
	else
		saveFile(request);
	if (this->_stat_code_no != 200 && this->_stat_code_no != 201) 
		setErrorFilePath(request);
}

PostResponse::~PostResponse()
{
	
}

void PostResponse::setHeaders(const HttpRequest& request)
{
	// this->headers["server"] = "localhost";
	// this->headers["content-Type"] = request.get_header("accept");
	this->_headers["content-Type"] = "text/html; charset=UTF-8";
	this->_headers["connection"] = request.getHeaders("connection");
}




void	PostResponse::saveFile(const HttpRequest& request)
{
	std::string upload_file_path = "/www/uploaded/" + request.getFilename();

	std::ofstream outputFile(upload_file_path, std::ios::binary);
	if (outputFile.is_open())
	{
		outputFile.write(request.getBody().c_str(), request.getBody().size());
		outputFile.close();
		{
			this->_stat_code_no = 201;
			return;
		}
	}
	else
	{
		this->_stat_code_no = 500;// TODO: figure out which error to give
		std::cerr << "Failed to save file: " << request.getFilename() << std::endl;
	}
}

void	PostResponse::postForm(const HttpRequest& request)
{
	if (request.getBody() == "")
		return;
}








// int	HttpResponse::processPost(int& status_code_no, const HttpRequest& request)
// {
// 	if (this->_stat_code_no != 200)
// 		return (status_code_no);
// 	// _file_path += request.getFileExtension();
// 	std::ofstream	post_file(this->_file_path);
// 	if (post_file.is_open())
// 	{
// 		post_file << request.getBody();
// 		post_file.close();
// 		return (201);
// 	}
// 	else
// 	{
// 		//TODO: ERROR
// 	}

// 	return (201);
// }

// int	HttpResponse::processDelete(int& status_code_no, const HttpRequest& request)
// {
// 	if (status_code_no != 200)
// 		std::cout << status_code_no << "=============== here ================== \n";
// 		return (status_code_no);
// 	if (remove(this->getFilePath().c_str()) != 0)
// 		return (404);
// 	if (request.getMethod() == "POST")//REMOVE=================================================
// 		return (200);
// 	return (200);
// }


