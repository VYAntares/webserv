#pragma once

#include "../config/ConfigStruct.hpp"
#include "ARequestHandler.hpp"

class ErrorHandler : public ARequestHandler {
	public:
		ErrorHandler(const Server& server, int code);
		ErrorHandler(const Location& loc, int code);
		ErrorHandler(const Location& loc, int code, std::string& body);
		~ErrorHandler() {};
};
