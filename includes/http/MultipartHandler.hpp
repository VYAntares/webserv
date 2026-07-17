#pragma once

#include "HttpRequest.hpp"
#include "ARequestHandler.hpp"

class MultipartHandler : public ARequestHandler {
	public:
		MultipartHandler(const Location& loc, const HttpRequest& req, const std::string& path);
		~MultipartHandler() {};

		void				setBody(const std::string& b);

	private:
		const HttpRequest*	_req;
		const std::string	_path;
};

