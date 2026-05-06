#pragma once

#include "IRequestHandler.hpp"

class CGIHandler : public IRequestHandler {
	public:
		CGIHandler();
		~CGIHandler();
		std::string buildResponse();
};
