#pragma once

#include "ARequestHandler.hpp"

class CGIHandler : public ARequestHandler {
	public:
		CGIHandler();
		~CGIHandler();
		std::string buildResponse();
};
