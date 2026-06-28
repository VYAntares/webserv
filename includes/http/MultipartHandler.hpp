#pragma once

#include "IRequestHandler.hpp"

class Multiparthandler : public IRequestHandler {
	public:
		MultipartHandler();
		~MultipartHandler();
		std::string buildResponse();
};