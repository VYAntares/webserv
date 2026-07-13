#pragma once

#include "../config/ConfigStruct.hpp"
#include "ARequestHandler.hpp"

class ErrorHandler : public ARequestHandler {
	public:
		ErrorHandler(const BaseBlock& b, int code, std::string body);
		~ErrorHandler() {};
};

