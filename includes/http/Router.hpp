#pragma once

#include <iostream>
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/config/ConfigStruct.hpp"


class Router {
    public:
        static int routeChecker(const HttpRequest& req, const Server& server);
    private:
        Router() {};
        ~Router() {};

        static int  bestRouteFound(const std::string& uri, const Server& server);
        static bool methodImplemented(const std::string& method);
        static int  methodAllowed(const std::string& method, const std::string& uri);
};