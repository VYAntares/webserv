#include "../../includes/http/StaticHandler.hpp"
#include "../../includes/utils/utils.hpp"
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <dirent.h>

StaticHandler::StaticHandler(const HttpRequest& req, const Location& loc, const std::string& path): _req(&req), _loc(&loc), _path(path) {
	_ncode = 200;
	_type = "";

	std::map<int, std::string>::const_iterator it = loc.error_page.find(_ncode);
    if (it != loc.error_page.end()) {
     	_errorpage = it->second;
	}
	if (loc.return_path.first != -1) {
		handleReturn(loc.return_path);
		return ;
	}
	if (isDir(path) && req.method != "POST") {
		if (path[path.length() - 1] != '/') {
			const std::pair<int, std::string> toreturn(301,path + '/');
			handleReturn(toreturn);
		} else
			throwList();
		return ;
	}
	if (req.method == "GET")
		handleGet();
	else if (req.method == "POST")
		handlePost();
	else if (req.method == "DELETE")
		handleDelete();
}



StaticHandler::StaticHandler(const Location& loc, int code, const std::string& body) {
	_ncode = code;
	_body = body;

	std::map<int, std::string>::const_iterator it = loc.error_page.find(_ncode);
    if (it != loc.error_page.end()) {
     	_errorpage = it->second;
	}
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



void	StaticHandler::throwList() {
	std::string path = _loc->root + _req->uri;
	if (path[path.length() - 1] != '/')
		path = path + '/';

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
	// _body += "<link rel=\"stylesheet\" href=\"/css/style.css\">";
	_body += "<title>Index of " + path + "</title>\n";
	_body += "</head>\n";
	_body += "<body>\n";
	_body += "<h1>Index of " + path + "</h1>\n";
	_body += "<ul>\n";
}

