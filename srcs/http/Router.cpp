#include "../../includes/http/Router.hpp"
#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/http/MultipartHandler.hpp"
#include "../../includes/http/StaticHandler.hpp"
#include "../../includes/http/ErrorHandler.hpp"
#include "../../includes/handlers/CGIHandler.hpp"
#include <unistd.h>

ARequestHandler*	Router::route(const HttpRequest& req, const Server& server,
									const std::string& peerAddr, IResponseSink* sink) {
	if (req.error != 200)
		return new ErrorHandler(server, req.error, "");
	
	// matcher la location sur le chemin SANS la query string : avec elle,
	// "/upload?x=1" ne matchait jamais "/upload" et retombait sur "/"
	std::string uriPath = req.uri.substr(0, req.uri.find('?'));

	const Location *loc = bestRouteFound(req.uri, server);
	if (!loc)
	return new ErrorHandler(server, 404, "");

	if (!methodImplemented(req.method))
		return new ErrorHandler(server, 501, "");

	if (!methodAllowed(req.method, loc))
		return new ErrorHandler(*loc, 405, "");

	// la redirection s'applique avant tout accès au filesystem : sinon un
	// "return" sur une URL sans fichier réel renvoyait 404 au lieu de rediriger
	if (loc->return_path.first != -1)
		return new StaticHandler(req, *loc, "");

	std::string path = resolvePath(loc, uriPath);
	if (path.empty())  // le chemin normalisé sort de root (ex: /../..)
		return new ErrorHandler(*loc, 403, "");

	// upload : si un dossier de stockage est configuré, c'est lui qui reçoit
    // les fichiers (exigence du sujet), pas root + URI
	if (req.method == "POST" && !loc->upload_store.empty()) {
		if (req.isMultipart == true)
			path = loc->upload_store;
		else {
			size_t slash = uriPath.rfind('/');
			std::string base = (slash == std::string::npos) ? uriPath : uriPath.substr(slash + 1);
			if (!base.empty())
				path = loc->upload_store + "/" + base;
		}
	}

	if (!fileExist(path, req.method))
		return new ErrorHandler(*loc, 404, "");

	if (forbiddenAccess(path, req.method))
		return new ErrorHandler(*loc, 403, "");

	std::string interpreter = isCgi(uriPath, loc);
	if (!interpreter.empty() && (req.method == "GET" || req.method == "POST")) {
		CGIHandler cgi(req, path, interpreter, peerAddr, loc, sink);
		return NULL;
	}

	if (req.method == "POST" && req.isMultipart == true)
		return new MultipartHandler(*loc, req, path);

	return new StaticHandler(req, *loc, path);
}



std::string Router::isCgi(const std::string& uri, const Location* loc) {
	size_t pos = uri.rfind('.');
	std::string ext;
	if (pos == std::string::npos)
		return "";
	else 
		ext = uri.substr(pos);
	if (loc->cgi_pass.find(ext) != loc->cgi_pass.end())
		return loc->cgi_pass.find(ext)->second;
	return "";
}



const std::string Router::resolvePath(const Location *loc, const std::string& uri) {
	bool    	slash = (uri[uri.length() - 1] == '/');
	std::string path  = normalizePath(loc->root + uri, loc->root);

	if (path.empty())
		return "";
	if (slash || (!slash && isDir(path)))
		path = path + "/";
	// std::cout << "slash > " << slash << "et isdir > " << isDir(path) << std::endl;

	if (path[path.length() - 1] == '/') {
		if (!loc->index.empty())
			path = path + "/" + loc->index;
		else if (!isDir(path))
			return "";
	}
	std::cout << "PATH > " << path << std::endl;
	return path;
}



int Router::fileExist(const std::string& path, const std::string& method) {
	if (fileFound(path) == false && method != "POST")
		return 0;
	return 1;
}



int Router::forbiddenAccess(const std::string& uri, const std::string& method) {
	int			mode = 0;
	std::string	pathtoaccess = uri;

	if (method == "DELETE")
		mode = W_OK;
	else if (method == "GET")
		mode = R_OK;
	else if (method == "POST")
		pathtoaccess = getParentDirectory(uri);
	if (access(pathtoaccess.c_str(), mode) == -1)
		return 1;
	return 0;
}



const Location* Router::bestRouteFound(const std::string& uri, const Server& server) {
	return findLocation(uri, server);
}



bool Router::methodImplemented(const std::string& method) {
	return (method == "GET" || method == "DELETE" || method == "POST" || method == "HEAD");
}



int Router::methodAllowed(const std::string& method, const Location *loc) {
	if (loc->allowed_methods.size() == 0)
		return 1;
	for (size_t i = 0; i < loc->allowed_methods.size(); i++)
		if (method == loc->allowed_methods[i])
			return 1;
	return 0;
}

