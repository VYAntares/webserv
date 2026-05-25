#include "../../includes/http/StaticHandler.hpp"
#include <sstream>

StaticHandler::StaticHandler(const HttpRequest& req, const Location& loc): _req(&req), _loc(&loc) {
	// const char *method = req.method.c_str();
	// switch(method) {
	// 	case("GET"):
	// 		handleGet();
	// 	case("POST"):
	// 		handlePost();
	// 	case("DELETE"):
	// 		handleDelete();
	// }
	// if (!loc->return.empty())
	// 		_ncode = loc->return;
	(void)req;
	(void)loc;
	_ncode = 200;
}

StaticHandler::~StaticHandler() {}

std::string	StaticHandler::getReason() {
	switch (_ncode) {
		case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
        default:  return "Unknown";
	}
}

std::string StaticHandler::buildResponse() {
    std::string body = "static handler response";
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _ncode << " " << getReason() << "\r\n"
		<< "Content-Type: text/plain\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return oss.str();
}

