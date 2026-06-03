#include "../../includes/http/ErrorHandler.hpp"
#include <sstream>

ErrorHandler::ErrorHandler(const Location *loc, int code) : _error(code){
	// le but c'est de return celui de la config si existant, sinon l'error code de la exec
	(void)loc;
	// if (loc->return)
	// 		_error(loc_return);
	// else
	// 		_error(code);
	// std::cout << "return code from config " << loc->return_path << std::endl;
}

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
		default:  return "Unknown";
	}
}

std::string itos(int n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::string ErrorHandler::buildResponse() {
    std::string reason = getReason();
	std::string body = "<html><body><h1>" + itos(_error) + " " + getReason() + "</h1></body></html>";
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _error << " " << reason << "\r\n"
		<< "Content-Type: text/html\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return oss.str();
}

