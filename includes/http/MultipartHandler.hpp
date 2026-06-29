#pragma once

#include "ARequestHandler.hpp"

class Multiparthandler : public ARequestHandler {
	public:
		MultipartHandler();
		~MultipartHandler();
		std::string buildResponse();
};