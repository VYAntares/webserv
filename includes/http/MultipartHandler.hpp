#pragma once

#include "IRequestHandler.hpp"

class Multiparthandler : public IRequestHandler {
	public:
		MultipartHandler();
		~MultipartHandler();

	// private:
	// 	const HttpRequest*	_req;
	// 	const Location*		_loc;
};