#pragma once

class IRequestHandler {
	virtual std::string buildResponse() = 0;
	virtual 			~IRequestHandler();
};
