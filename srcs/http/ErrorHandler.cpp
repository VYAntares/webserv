#include "../../includes/http/ErrorHandler.hpp"
#include <sstream>

ErrorHandler::ErrorHandler(int code) {
	(void)code;
}

ErrorHandler::~ErrorHandler() {}


std::string ErrorHandler::buildResponse() {
    std::string body = "Error handler response";
	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/plain\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return oss.str();
}

