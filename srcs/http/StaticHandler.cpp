#include "../../includes/http/StaticHandler.hpp"
#include <sstream>

StaticHandler::StaticHandler() {}

StaticHandler::~StaticHandler() {}

std::string StaticHandler::buildResponse() {
    std::string body = "static handler response";
	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/plain\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return oss.str();
}

