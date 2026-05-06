#pragma once

#include "IRequestHandler.hpp"
#include <string>
#include <iostream>

class StaticHandler : public IRequestHandler {
	public:
		StaticHandler();
		~StaticHandler();
		std::string buildResponse();
};

