#include "../../includes/http/CGIHandler.hpp"
#include <sstream>

CGIHandler::CGIHandler() {}

CGIHandler::~CGIHandler() {}

std::string CGIHandler::buildResponse() {
    std::string body = "CGI handler response";
	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/plain\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return oss.str();
}

