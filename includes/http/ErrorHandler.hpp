#pragma once

#include "../config/ConfigStruct.hpp"
#include "IRequestHandler.hpp"
#include "../utils/utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

class ErrorHandler : public IRequestHandler {
	public:
		ErrorHandler(const Server& server, int code);
		ErrorHandler(const Location& loc, int code);
		~ErrorHandler() {};
};
