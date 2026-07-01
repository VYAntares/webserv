#pragma once

#include "../config/ConfigStruct.hpp"
#include "ARequestHandler.hpp"
#include "../utils/utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

class ErrorHandler : public ARequestHandler {
	public:
		ErrorHandler(const Server& server, int code);
		ErrorHandler(const Location& loc, int code);
		~ErrorHandler() {};
};
