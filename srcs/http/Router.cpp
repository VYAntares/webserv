#include "../../includes/http/Router.hpp"
#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/http/StaticHandler.hpp"
#include "../../includes/http/ErrorHandler.hpp"
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <cstdio>

IRequestHandler*	Router::route(const HttpRequest& req, const Server& server) {
    if (req.error != 200)
		return new ErrorHandler(req.error);

    const Location *loc = bestRouteFound(req.uri, server);
    if (!loc)
        return new ErrorHandler(loc, 404);

    std::string path = resolvePath(loc, req.uri);
    if (path.empty())
        return new ErrorHandler(loc, 403);

    if (!methodImplemented(req.method))
        return new ErrorHandler(loc, 501);

    if (!fileFound(path, req.method))
        return new ErrorHandler(loc, 404);

    if (forbiddenAccess(path, req.method))
        return new ErrorHandler(403);

    if (!methodAllowed(req.method, loc))
        return new ErrorHandler(405);

	//if (isCgi())
    
    return new StaticHandler(req, *loc, path);
}

const std::string Router::resolvePath(const Location *loc, const std::string& uri) {
    std::string path = loc->root + uri;
    std::string newpath = path;
    if (path[path.length() - 1] == '/') {
        if (!loc->index.empty())
            newpath = path + loc->index;
        else {
            if (loc->autoindex == 1)
                // regarder comment on renvoie la liste
                std::cout << "throw list" << std::endl;
            else
                return "";
        }
    }
    return newpath;
}

int Router::fileFound(const std::string& path, const std::string& method) {
    struct stat forbuf;
    
    if (method == "POST")
        return 1;
    if (stat(path.c_str(), &forbuf) == -1)
        return 0;
    return 1;
}

int Router::forbiddenAccess(const std::string& uri, const std::string& method) {
    int mode = 0; 

    if (method == "DELETE" || method == "POST")
        mode = W_OK;
    else if (method == "GET")
        mode = R_OK;
    if (access(uri.c_str(), mode) == -1)
        return 1;
    return 0;
}

const Location* Router::bestRouteFound(const std::string& uri, const Server& server) {
    std::string shorturi = uri;
    const Location *loc = NULL; 
    int len = -1;
    if (uri[0] != '/')
        return NULL;
    while (shorturi.length() != 0) {
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
        size_t i = shorturi.rfind('/');
        shorturi.erase(i);
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
