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
		return new ErrorHandler(server, req.error);
    
    
    // matcher la location sur le chemin SANS la query string : avec elle,
    // "/upload?x=1" ne matchait jamais "/upload" et retombait sur "/"
    std::string uriPath = req.uri.substr(0, req.uri.find('?'));

	const Location *loc = bestRouteFound(req.uri, server);
    if (!loc)
    return new ErrorHandler(server, 404);

    if (!methodImplemented(req.method))
        return new ErrorHandler(server, 501);

    if (!methodAllowed(req.method, loc))
        return new ErrorHandler(*loc, 405);

    std::string path = resolvePath(loc, uriPath);
	if (path.empty())  // le chemin normalisé sort de root (ex: /../..)
		return new ErrorHandler(*loc, 403);
	
	if (!fileExist(path, req.method))
		return new ErrorHandler(*loc, 404);

	if (forbiddenAccess(path, req.method))
		return new ErrorHandler(*loc, 403);

	std::string interpreter = isCgi(uriPath, loc);
	if (!interpreter.empty()) {
        CGIHandler cgi(req, path, interpreter, peerAddr, loc, sink);
		return NULL;
	}

    if (req.method == "POST" && req.isMultipart == true)
        return new MultipartHandler(req, path);

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
    std::string path = normalizePath(loc->root + uri, loc->root);
    if (path.empty())
        return "";
    std::string newpath = path;
    if (path[path.length() - 1] == '/') {
        if (!loc->index.empty())
            newpath = newpath + loc->index;
        else if (!isDir(newpath))
            return "";
    }
    return newpath;
}



int Router::fileExist(const std::string& path, const std::string& method) {
    if (fileFound(path) == false && method != "POST")
        return 0;
    return 1;
}



int Router::forbiddenAccess(const std::string& uri, const std::string& method) {
    int mode = 0;
    std::string pathtoaccess = uri;

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

