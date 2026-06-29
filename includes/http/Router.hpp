#pragma once

#include <iostream>
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/http/StaticHandler.hpp"
#include "../../includes/http/ErrorHandler.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/utils/utils.hpp"
#include "ARequestHandler.hpp"

class Router {
    public:
		static ARequestHandler* route(const HttpRequest& req, const Server& server);

    private:
        Router() {};
        ~Router() {};

        static const Location*   		bestRouteFound(const std::string& uri, const Server& server);
        static bool 					methodImplemented(const std::string& method);
        static int 						fileExist(const std::string& uri, const std::string& method);
        static int 						forbiddenAccess(const std::string& uri, const std::string& method);
        static const std::string		resolvePath(const Location *loc, const std::string& uri);
        static int 						methodAllowed(const std::string& method, const Location *loc);
		static std::string				isCgi(const std::string& uri, const Location* loc);
};
