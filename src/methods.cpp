// #include "Server.hpp"
// #include "HTTPRequest.hpp"
// #include <fstream>
// #include <iostream>

// void Server::handleGet(int clientSock, HttpRequest& httpRequest) {
//     std::string filePath = resolvePath(httpRequest.uri);
//     if (filePath == ROOT_DIR) filePath += "index.html";

//     // Check if file exists
//     std::ifstream file(filePath);
//     if (!file.good()) {
//         sendResponse(clientSock, "404 Not Found", 404, "text/plain");
//     } else {
//         try {
//             std::string content((std::istreambuf_iterator<char>(file)),
//                                 std::istreambuf_iterator<char>());
//             std::string contentType = getMimeType(filePath);
//             sendResponse(clientSock, content, 200, contentType);
//         } catch (const std::exception& e) {
//             sendResponse(clientSock, "500 Internal Server Error", 500, "text/plain");
//         }
//     }
// }
// void Server::handleDelete(int clientSock, HttpRequest& httpRequest) {
//     try {
//         std::string filePath = resolvePath(httpRequest.uri);

//         // Check if file exists before attempting to delete
//         std::ifstream file(filePath);
//         if (!file.good()) {
//             sendResponse(clientSock, "404 Not Found", 404, "text/plain");
//         } else {
//             file.close();
//             if (remove(filePath.c_str()) != 0) {
//                 throw std::runtime_error("Failed to delete file");
//             }
//             sendResponse(clientSock, "Resource deleted", 200, "text/plain");
//         }
//     } catch (const std::exception& e) {
//         sendResponse(clientSock, "500 Internal Server Error", 500, "text/plain");
//     }
// }
// void Server::handlePost(int clientSock, HttpRequest& httpRequest) {
//     // Send 100 Continue if expected
//     auto expectHeader = httpRequest.headers.find("expect");
//     if (expectHeader != httpRequest.headers.end()) {
//         std::string expectValue = expectHeader->second;
//         std::transform(expectValue.begin(), expectValue.end(), expectValue.begin(), ::tolower);
//         if (expectValue == "100-continue") {
//             std::string continueResponse = "HTTP/1.1 100 Continue\r\n\r\n";
//             write(clientSock, continueResponse.c_str(), continueResponse.size());
//         }
//     }

//     // Get Content-Length
//     auto contentLengthIt = httpRequest.headers.find("content-length");
//     if (contentLengthIt == httpRequest.headers.end()) {
//         sendResponse(clientSock, "411 Length Required", 411, "text/plain");
//         return;
//     }
//     size_t contentLength = std::stoul(contentLengthIt->second);
// 	auto contentTypeIt = httpRequest.headers.find("content-type");
//     if (contentTypeIt == httpRequest.headers.end()) {
//         sendResponse(clientSock, "400 Bad Request: Content-Type header missing", 400, "text/plain");
//         return;
//     }
//     // Read the remaining body if necessary
//     size_t bodyAlreadyRead = httpRequest.body.size();
//     if (bodyAlreadyRead < contentLength) {
//         std::string requestBody = httpRequest.body; // Copy any already read body content
//         size_t totalBytesRead = bodyAlreadyRead;

//         while (totalBytesRead < contentLength) {
//             char buffer[4096];
//             ssize_t bytesRead = read(clientSock, buffer, std::min(sizeof(buffer), contentLength - totalBytesRead));
//             if (bytesRead < 0) {
//                 if (errno == EAGAIN || errno == EWOULDBLOCK) {
//                     usleep(1000); // Sleep for 1 millisecond
//                     continue;
//                 } else if (errno == EINTR) {
//                     continue;
//                 } else {
//                     int err = errno;
//                     std::cerr << "Read error: " << strerror(err) << " (errno " << err << ")\n";
//                     sendResponse(clientSock, "500 Internal Server Error: Read error", 500, "text/plain");
//                     return;
//                 }
//             } else if (bytesRead == 0) {
//                 sendResponse(clientSock, "400 Bad Request: Incomplete body", 400, "text/plain");
//                 return;
//             }
//             requestBody.append(buffer, bytesRead);
//             totalBytesRead += bytesRead;
//         }
//         httpRequest.body = requestBody; // Update the request body with the full content
//     }

//     // Now process the request body
//     std::cout << "Request Body Length: " << httpRequest.body.size() << "\n";

//     std::string contentType = contentTypeIt->second;
//     // Trim leading and trailing whitespace
//     contentType.erase(0, contentType.find_first_not_of(" \t\r\n"));
//     contentType.erase(contentType.find_last_not_of(" \t\r\n") + 1);
//     // Convert to lowercase
//     std::transform(contentType.begin(), contentType.end(), contentType.begin(), ::tolower);
//     std::cout << "Normalized Content-Type: '" << contentType << "'\n";

//     if (contentType.find("multipart/form-data") != std::string::npos) {
//         // Handle multipart/form-data (file upload)
//         std::cout << "Entering file upload handling block.\n";

//         size_t boundaryPos = contentType.find("boundary=");
//         if (boundaryPos == std::string::npos) {
//             sendResponse(clientSock, "400 Bad Request: Boundary missing in Content-Type", 400, "text/plain");
//             return;
//         }

//         std::string boundary = "--" + contentType.substr(boundaryPos + 9);
//         boundary.erase(boundary.find_last_not_of(" \t\n\r") + 1); // Trim whitespace
//         std::cout << "Extracted Boundary: " << boundary << "\n";

//         std::string boundaryInBody = httpRequest.body.substr(0, httpRequest.body.find("\r\n"));
//         std::cout << "Boundary in Body: " << boundaryInBody << "\n";

//         if (boundaryInBody != boundary) {
//             sendResponse(clientSock, "400 Bad Request: Boundary mismatch", 400, "text/plain");
//             return;
//         }

//         // Locate the file content in the multipart body
//         size_t fileHeaderStart = httpRequest.body.find("\r\n", boundary.length()) + 2;
//         size_t fileHeaderEnd = httpRequest.body.find("\r\n\r\n", fileHeaderStart);
//         if (fileHeaderEnd == std::string::npos) {
//             sendResponse(clientSock, "400 Bad Request: Invalid multipart headers", 400, "text/plain");
//             return;
//         }

//         size_t fileContentStart = fileHeaderEnd + 4;
//         size_t boundaryPosInBody = httpRequest.body.find(boundary, fileContentStart);
//         if (boundaryPosInBody == std::string::npos) {
//             sendResponse(clientSock, "400 Bad Request: Invalid multipart form data", 400, "text/plain");
//             return;
//         }
//         size_t fileContentEnd = boundaryPosInBody - 2; // Adjust as needed

//         std::string fileContent = httpRequest.body.substr(fileContentStart, fileContentEnd - fileContentStart);
//         std::cout << "Extracted File Content Length: " << fileContent.size() << "\n";

//         // Save the file to the uploads directory
//         std::string filePath = "www/uploads/uploaded_file.jpg";
//         std::ofstream outFile(filePath, std::ios::binary);
//         if (!outFile.is_open()) {
//             sendResponse(clientSock, "500 Internal Server Error: Unable to save file", 500, "text/plain");
//             return;
//         }
//         outFile.write(fileContent.c_str(), fileContent.size());
//         outFile.close();

//         sendResponse(clientSock, "File uploaded successfully!", 200, "text/plain");
//         std::cout << "File uploaded successfully.\n";
//     } else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
//         // Handle form data
//         std::cout << "Entering form data handling block.\n";

//         // Parse form data
//         std::string name = parseName(httpRequest.body);
//         std::string content = parseContent(httpRequest.body);

//         std::cout << "Parsed Name: " << name << "\n";
//         std::cout << "Parsed Content: " << content << "\n";

//         // Save the data to a file
//         std::string dataFilePath = "www/data/form_data.txt";
//         std::cout << "Attempting to open file: " << dataFilePath << "\n";
//         std::ofstream dataFile(dataFilePath, std::ios::app);
//         if (!dataFile.is_open()) {
//             std::cerr << "Failed to open file: " << dataFilePath << "\n";
//             perror("Error opening file");
//             sendResponse(clientSock, "500 Internal Server Error: Unable to save form data", 500, "text/plain");
//             return;
//         }
//         std::cout << "File opened successfully.\n";

//         dataFile << "Name: " << name << "\n";
//         dataFile << "Content: " << content << "\n";
//         dataFile << "--------------------------\n";
//         dataFile.close();
//         std::cout << "Data saved to file successfully.\n";

//         // Send a response
//         std::cout << "Sending response to client...\n";
//         sendResponse(clientSock, "Form data received and saved", 200, "text/plain");
//         std::cout << "Response sent to client.\n";
//     } else {
//         // Handle other content types or send an error
//         sendResponse(clientSock, "415 Unsupported Media Type", 415, "text/plain");
//     }
// }