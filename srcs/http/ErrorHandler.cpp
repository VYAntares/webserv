#include "../../includes/http/ErrorHandler.hpp"
#include <sstream>
#include <fstream>

static std::map<std::string, std::string> mime_types = init_mime_types();

ErrorHandler::ErrorHandler(const Location& loc, int code) : _error(code){
	// le but c'est de return celui de la config si existant, sinon l'error code de la exec
	// (void)loc;
	if (loc.return_path.first != -1)
		handleReturn(loc.return_path);

	std::map<int, std::string>::const_iterator it = loc.error_page.find(_error);

	if (it != loc.error_page.end())
    	_errorpage = it->second;
	// if (loc->return)
	// 		_error(loc_return);
	// else
	// 		_error(code);
	// std::cout << "return code from config " << loc->return_path << std::endl;
}

ErrorHandler::ErrorHandler(const Server& server, int code) : _error(code){
	if (server.return_path.first != -1) {
		handleReturn(server.return_path);
		return ;
	}
	std::map<int, std::string>::const_iterator it = server.error_page.find(_error);

	if (it != server.error_page.end())
		_errorpage = it->second;
}

void ErrorHandler::handleReturn(const std::pair<int, str>& return_path) {
	_error = return_path.first;

	if (!return_path.second.empty()) {
		_body = "<html><body><h1>Redirecting to " + return_path.second + "</html></body></h1>";
		_location = return_path.second;
	}
	else
		_body = "<html><body><h1>Redirecting</html></body></h1>";
	_type = getType(".html");
}

ErrorHandler::~ErrorHandler() {}

std::string	ErrorHandler::getReason() {
	switch (_error) {
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

std::string ErrorHandler::getType(const std::string& path) {
	std::string	ext;

	size_t pos = path.rfind(".");
	if (pos == std::string::npos)
		return "application/octet-stream";

	ext = path.substr(pos);
	
	if (mime_types.count(ext))
		return mime_types[ext];

	return "application/octet-stream";
}

void	ErrorHandler::getErrorPage() {
	_type = getType(_errorpage);

	std::ifstream file(_errorpage.c_str());
	if (!file.is_open())
		return ;

	std::stringstream ss;
	ss << file.rdbuf();
	_body = ss.str();
	file.close();
}

std::string itos(int n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::string ErrorHandler::buildResponse() {
    std::string reason = getReason();
	if (_errorpage.length() > 0)
		getErrorPage();
	else
		_body = "<html><body><h1>" + itos(_error) + " " + getReason() + "</h1></body></html>";
	
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _error << " " << reason << "\r\n";

	if ((_error >= 300 && _error < 400) && !_location.empty())
		oss << "Location: " << _location << "\r\n";

	oss	<< "Content-Type: " << _type << "\r\n"
		<< "Content-Length: " << _body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< _body;

	return oss.str();
}

