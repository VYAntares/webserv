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
        return new ErrorHandler(404);

    // std::string finalPath = resolvePath(&loc, req.uri);
    if (!methodImplemented(req.method))
        return new ErrorHandler(501);
    // tant qu'on demande des files qui n'existent pas sort en erreur
    // if (!fileFound(req.uri, req.method))
    //     return new ErrorHandler(404);

    // if (forbiddenAccess(req.uri, req.method))
    //     return new ErrorHandler(403);

    // if (methodAllowed(&loc, req.method))
    //     return new ErrorHandler(405);

	//if (isCgi())
    return new StaticHandler();
}

// const std::string Router::resolvePath(const Location *loc, const std::string& uri) {

// }

int Router::fileFound(const std::string& uri, const std::string& method) {
    struct stat forbuf;

    if (method == "POST")
        return 1;
    if (stat(uri.c_str(), &forbuf) == -1)
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

// int Router::methodAllowed(const std::string& method, const std::string& uri) {
//     std::cout << "\n\n\nIN methodAllowed" << uri << method << std::endl;
//     // need config oops
//     return 1;
// }
