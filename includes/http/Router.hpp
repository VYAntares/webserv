#pragma once

#include <iostream>
#include "IRequestHandler.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/config/ConfigStruct.hpp"

class Router {
    public:
		static IRequestHandler* route(const HttpRequest& req, const Server& server);

    private:
        Router() {};
        ~Router() {};

        static const Location*    bestRouteFound(const std::string& uri, const Server& server);
        static bool 			        methodImplemented(const std::string& method);
        static int                fileFound(const std::string& uri, const std::string& method);
        static int                forbiddenAccess(const std::string& uri, const std::string& method);
        // static const std::string  resolvePath(const Location *loc, const std::string& uri);

};

