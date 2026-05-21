#pragma once

#include "IRequestHandler.hpp"
#include <string>
#include <iostream>
#include <sstream>

class ErrorHandler : public IRequestHandler {
	public:
		ErrorHandler(int code);
		~ErrorHandler();
		std::string	buildResponse();
		std::string	getReason();
	
	private:
		int	_error;
};

