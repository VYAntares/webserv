#include "../../includes/http/StaticHandler.hpp"

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

std::string StaticHandler::buildResponse() {
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _ncode << " " << getReason(_ncode) << "\r\n";

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
	std::ofstream	file(_path.c_str());
	std::string 	body = _req->body;
	bool			exist = fileFound(_path);

	size_t pos = _req->body.find('+');
	size_t pos2 = _req->body.find('%');

	if (pos != std::string::npos || pos2 != std::string::npos)
		body = decodeHexa(_req->body, 1);
	
	file << body;

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