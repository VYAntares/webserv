#include "../../includes/http/Router.hpp"

int Router::routeChecker(const HttpRequest& req, const Server& server)
{
    if (!methodImplemented(req.method))
        return 501;

    if (bestRouteFound(req.uri, server))
        return 404;

    if (!methodAllowed(req.method, req.uri))
        return 405;

    return 200;
}

int Router::bestRouteFound(const std::string& uri, const Server& server) {
    (void)uri;
    for (size_t i =0; i < server.locations.size(); i++) {
        std::cout << "server loc " << server.locations[i].path << std::endl;
    }
    return 1;
}

bool Router::methodImplemented(const std::string& method) {
    return (method == "GET" || method == "DELETE" || method == "POST");
}

int Router::methodAllowed(const std::string& method, const std::string& uri) {
    std::cout << uri << method << std::endl;
    // need config oops
    return 1;
}