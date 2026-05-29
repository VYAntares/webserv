#include "../../includes/http/StaticHandler.hpp"
#include <sstream>
#include <fstream>

std::map<std::string, std::string> mime_types = init_mime_types();

StaticHandler::StaticHandler(const HttpRequest& req, const Location& loc, const std::string& path): _req(&req), _loc(&loc), _type(""), _path(path) {
	_ncode = 200;

	if (!loc.return_path.first || !loc.return_path.second.empty()) {
		handleReturn();
		return ;
	}

	if (req.method == "GET")
		handleGet();
	else if (req.method == "POST")
		handlePost();
	else if (req.method == "DELETE")
		handleDelete();
}

StaticHandler::~StaticHandler() {}

std::string	StaticHandler::getReason() {
	switch (_ncode) {
		case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
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

std::string StaticHandler::buildResponse() {
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _ncode << " " << getReason() << "\r\n"
		<< "Content-Type: " << _type << "\r\n"
		<< "Content-Length: " << _body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< _body;
	return oss.str();
}

std::string StaticHandler::getType(const std::string& path) {
	std::string	ext;

	size_t pos = path.rfind(".");
	if (pos == std::string::npos)
		return "application/octet-stream";

	ext = path.substr(pos);
	
	if (mime_types.count(ext))
		return mime_types[ext];

	return "application/octet-stream";
}

void	StaticHandler::handleGet() {

	_type = getType(_path);

	std::ifstream file(_path.c_str());
	if (!file.is_open())
		return ;

	std::stringstream ss;
	ss << file.rdbuf();
	_body = ss.str();
	file.close();
}

void	StaticHandler::handlePost() {
	std::cout << "handle post" << std::endl;
}

void	StaticHandler::handleDelete() {
	std::cout << "handle delete" << std::endl;
}

void    StaticHandler::handleReturn() {
    _ncode = _loc->return_path.first;
    _type = getType(".html");
    _body = "<html><body><h1>Redirecting to " + _loc->return_path.second + "</html></body></h1>";
}