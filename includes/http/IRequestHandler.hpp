#pragma once

#include <string>
#include <iostream>

class IRequestHandler {
	public:
		virtual std::string buildResponse() = 0;
		virtual				~IRequestHandler() {}
};

