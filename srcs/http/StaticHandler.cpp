#include "../../includes/http/StaticHandler.hpp"
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h> 

static std::map<std::string, std::string> mime_types = init_mime_types();

StaticHandler::StaticHandler(const HttpRequest& req, const Location& loc, const std::string& path): _req(&req), _loc(&loc), _type(""), _path(path) {
	_ncode = 200;

	if (path.empty()) {
		throwList();
		return ;
	}

	if (loc.return_path.first != -1 || !loc.return_path.second.empty()) {
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
	oss << "HTTP/1.1 " << _ncode << " " << getReason() << "\r\n";

	if ((_ncode >= 300 && _ncode < 400 ) && !_location.empty())
		oss << "Location: " << _location << "\r\n";

	if (!_type.empty())
		oss	<< "Content-Type: " << _type << "\r\n";

	oss << "Content-Length: " << _body.size() << "\r\n"
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
	bool exist = fileFound(_path);

	std::ofstream file(_path.c_str());

	file << _req->body;

	if (!exist)
		_ncode = 201;
}

void	StaticHandler::handleDelete() {
	int res = unlink(_path.c_str());

	if (res == -1)
		_ncode = 404;
	else
		_ncode = 204;
}

void    StaticHandler::handleReturn() {
    _ncode = _loc->return_path.first;
    _type = getType(".html");
	if (!_loc->return_path.second.empty()) {
		_body = "<html><body><h1>Redirecting to " + _loc->return_path.second + "</html></body></h1>";
		_location = _loc->return_path.second;
	}
	else
		_body = "<html><body><h1>Redirecting</html></body></h1>";
}

void	StaticHandler::throwList() {
	std::string path = _loc->root + _req->uri;
	std::string html;
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return ;
	headerListe(path);
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string listing = entry->d_name;
		if (listing == "." || listing == "..")
			continue ;
		if (isDir(path + listing))
			html += "<li><a class=\"directory\" href=\"" + listing + "/\">" + listing + "</a><br>\n";
		else
			html += "<li><a class=\"file\" href=\"" + listing + "\">" + listing + "</a><br>\n";
	}
	closedir(dir);
	if (html.empty())
		_body += "<html><body>Empty directory";
	else
		_body += html + "</ul>\n";
	html += "</body>\n</html>\n";
	_type = getType(".html");
}

bool isDir(const std::string& path) {
	struct stat st;

	if (stat(path.c_str(), &st) == -1)
		return false;
	return S_ISDIR(st.st_mode);
}

void StaticHandler::headerListe(const std::string& path) {

	_body += "<!DOCTYPE html>\n";
	_body += "<html>\n";
	_body += "<head>\n";
	_body += "<link rel=\"stylesheet\" href=\"/css/style.css\">";
	_body += "<title>Index of " + path + "</title>\n";
	_body += "<link rel=\"stylesheet\" href=\"/style.css\">\n";
	_body += "</head>\n";
	_body += "<body>\n";
	_body += "<h1>Index of " + path + "</h1>\n";
	_body += "<ul>\n";
}

bool	fileFound(const std::string& path) {
	struct stat forbuf;

	if (stat(path.c_str(), &forbuf) == -1)
		return false;
	return true;
}