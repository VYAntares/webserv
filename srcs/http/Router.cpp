#include "../../includes/http/Router.hpp"
#include "../../includes/http/CGIHandler.hpp"

ARequestHandler*	Router::route(const HttpRequest& req, const Server& server,
									const std::string& peerAddr) {
    if (req.error != 200)
		return new ErrorHandler(server, req.error);

    const Location *loc = bestRouteFound(req.uri, server);
    if (!loc)
        return new ErrorHandler(server, 404);

    std::string uriPath = req.uri.substr(0, req.uri.find('?'));

    std::string path = resolvePath(loc, uriPath);
    if (path.empty() && loc->autoindex == 0)
        return new ErrorHandler(*loc, 403);

    if (!methodImplemented(req.method))
        return new ErrorHandler(*loc, 501);

    if (!fileExist(path, req.method))
        return new ErrorHandler(*loc, 404);

    if (!methodAllowed(req.method, loc))
        return new ErrorHandler(*loc, 405);

    if (forbiddenAccess(path, req.method))
        return new ErrorHandler(*loc, 403);

    if (!methodAllowed(req.method, loc))
		return new ErrorHandler(*loc, 405);

	std::string interpreter = isCgi(uriPath, loc);
	if (!interpreter.empty())
        return new CGIHandler(req, loc, path, interpreter, peerAddr);

    if (req.method == "POST" && req.isMultipart == true)
        return new MultipartHandler(req, *loc, path);

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
    std::string path = loc->root + uri;
    std::string newpath = path;
    if (path[path.length() - 1] == '/') {
        if (!loc->index.empty())
            newpath = path + loc->index;
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
    std::string shorturi = uri;
    const Location *loc = NULL; 
    int len = -1;
    if (uri[0] != '/')
        return NULL;
    while (true) {
        for (size_t i = 0; i < server.locations.size(); i++) {
            if (server.locations[i].path.find(shorturi) == 0 && 
                server.locations[i].path == shorturi && 
                len < (int)(server.locations[i].path.length())) {
                loc = &server.locations[i];
                len = (int)(server.locations[i].path.length());
            } 
        }
        if (len != -1)
            break;

        if (shorturi.empty())
            break;

        size_t i = shorturi.rfind('/');
        shorturi.erase(i);
        if (shorturi.empty())
            shorturi = "/";
    }
    return loc;
}

bool Router::methodImplemented(const std::string& method) {
    return (method == "GET" || method == "DELETE" || method == "POST");
}

int Router::methodAllowed(const std::string& method, const Location *loc) {
    if (loc->allowed_methods.size() == 0)
        return 1;
    for (size_t i = 0; i < loc->allowed_methods.size(); i++)
        if (method == loc->allowed_methods[i])
            return 1;
    return 0;
}
