#pragma once

#include "ARequestHandler.hpp"

class Multiparthandler : public ARequestHandler {
	public:
		MultipartHandler(const HttpRequest& req, const Location& loc, const std::string& path);
		~MultipartHandler() {};

		void    setBody(const std::string& b);

	private:
		const HttpRequest*	_req;
		const Location*		_loc;
		const std::string	_path;
};