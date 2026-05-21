#include "../../includes/http/ErrorHandler.hpp"
#include <sstream>

ErrorHandler::ErrorHandler(int code) : _error(code){
}

ErrorHandler::~ErrorHandler() {}

std::string	ErrorHandler::getReason() {
	switch (_error) {
		case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
		case 405: return "Method not allowed";
		case 413: return "Body size too large";
        case 500: return "Internal Server Error";
		case 501: return "Method not implemented";
		case 
        default:  return "Unknown";
	}
}

std::string ErrorHandler::buildResponse() {
    std::string reason = getReason();
	std::string body = "Error handler response" + reason;

	std::cout << "erreur " << _error << " et reason: " << reason << std::endl;
	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/plain\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return oss.str();
}

