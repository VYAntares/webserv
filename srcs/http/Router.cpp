#include "../../includes/http/Router.hpp"
#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/http/StaticHandler.hpp"
#include "../../includes/http/ErrorHandler.hpp"

IRequestHandler*	Router::route(const HttpRequest& req, const Server& server) {
	if (req.error != 200)
		return new ErrorHandler(req.error);

    if (!methodImplemented(req.method))
        return new ErrorHandler(501);

    if (!bestRouteFound(req.uri, server)) {
		std::cout << "ERROR 404" << std::endl;
        return new ErrorHandler(404);
	}

	//if (isCgi())

    return new StaticHandler();
}

const Location* Router::bestRouteFound(const std::string& uri, const Server& server) {
    std::string shorturi = uri;
    int j = -1;
    int len = -1;
    if (uri[0] != '/')
        return NULL;
    while (shorturi.length() != 0) {
        for (size_t i = 0; i < server.locations.size(); i++) {
            if (server.locations[i].path.find(shorturi) == 0 && 
                server.locations[i].path == shorturi && 
                len < (int)(server.locations[i].path.length())) {
                j = i;
                len = (int)(server.locations[i].path.length());
            } 
        }
        if (len != -1)
            break;
        size_t i = shorturi.rfind('/');
        shorturi.erase(i);
    }
    if (j == -1)
        return NULL;
    return &server.locations[j];
}

bool Router::methodImplemented(const std::string& method) {
    return (method == "GET" || method == "DELETE" || method == "POST");
}

// int Router::methodAllowed(const std::string& method, const std::string& uri) {
//     std::cout << "\n\n\nIN methodAllowed" << uri << method << std::endl;
//     // need config oops
//     return 1;
// }
