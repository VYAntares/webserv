#include "../../includes/http/StaticHandler.hpp"
#include "../../includes/utils/utils.hpp"
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <dirent.h>

StaticHandler::StaticHandler(const HttpRequest& req, const Location& loc, const std::string& path): _req(&req), _loc(&loc), _path(path) {
	_ncode = 200;
	_type = "";

	if (loc.return_path.first != -1) {
		handleReturn(loc.return_path);
		return ;
	}
	if (isDir(path) && req.method != "POST") {
		// rediriger sur l'URI, pas sur le chemin filesystem : avant, la
		// Location renvoyée était "/var/www/html/" → 404 chez le client
		std::string uriPath = req.uri.substr(0, req.uri.find('?'));
		if (uriPath[uriPath.length() - 1] != '/') {
			const std::pair<int, std::string> toreturn(301, uriPath + '/');
			handleReturn(toreturn);
		} else if (loc.autoindex == 1) // autoindex n'était jamais vérifié ici
			throwList();
		else
			_ncode = 403;
	}
	else if (req.method == "GET")
		handleGet();
	else if (req.method == "POST")
		handlePost();
	else if (req.method == "DELETE")
		handleDelete();

	// chercher la page d'erreur configurée pour le code FINAL : avant, la
	// recherche se faisait en début de constructeur avec _ncode encore à 200,
	// donc les error_page n'étaient jamais utilisées par ce handler
	std::map<int, std::string>::const_iterator it = loc.error_page.find(_ncode);
    if (it != loc.error_page.end())
     	_errorpage = it->second;
}

StaticHandler::StaticHandler(const Location& loc, int code, const std::string& body)
	: _req(NULL), _loc(&loc) { // pointeurs laissés non initialisés avant (UB latent)
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
	// le fichier existe (vérifié par le Router) mais ne s'ouvre pas :
	// droits insuffisants → 403, pas un faux 200 avec body vide
	if (!file.is_open()) {
		_ncode = 403;
		_type = "";
		return ;
	}

	std::stringstream ss;
	ss << file.rdbuf();
	_body = ss.str();
	file.close();
}

void	StaticHandler::handlePost() {
	// tester l'existence AVANT d'ouvrir : l'ofstream crée le fichier,
	// donc l'ancien ordre rendait exist toujours vrai → jamais de 201
	bool			exist = fileFound(_path);
	std::ofstream	file(_path.c_str());
	std::string 	body = _req->body;

	if (!file.is_open()) { // dossier cible absent ou non inscriptible
		_ncode = 403;
		return ;
	}

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
	// _path est déjà le chemin résolu et normalisé du dossier : le recalculer
	// avec root + uri ré-introduisait la query string et cassait l'opendir
	std::string path = _path;
	if (path[path.length() - 1] != '/')
		path = path + '/';

	std::string html;
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		_ncode = 403;
		return ;
	}
	headerListe(path);
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string listing = entry->d_name;
		if (listing == "." || listing == "..")
			continue ;
		if (isDir(path + listing))
			html += "<li><a class=\"directory\" href=\"" + listing + "/\">" + listing + "</a></li>\n";
		else
			html += "<li><a class=\"file\" href=\"" + listing + "\">" + listing + "</a></li>\n";
	}
	closedir(dir);
	if (html.empty())
		_body += "<li>Empty directory</li>\n";
	else
		_body += html;
	// les balises fermantes étaient ajoutées à la variable locale `html`
	// APRÈS sa copie dans _body : elles n'atteignaient jamais la réponse
	_body += "</ul>\n</body>\n</html>\n";
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
