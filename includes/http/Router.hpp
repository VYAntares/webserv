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

        static const Location*  bestRouteFound(const std::string& uri, const Server& server);
        static bool 			methodImplemented(const std::string& method);
};

