#pragma once

#include "IRequestHandler.hpp"
#include "HttpRequest.hpp"
#include "../config/ConfigStruct.hpp"
#include <string>
#include <iostream>

class StaticHandler : public IRequestHandler {
	public:
		StaticHandler(const HttpRequest& req, const Location& loc);
		~StaticHandler();
		std::string buildResponse();
		std::string	getReason();
		void		handleGet();
		void		handlePost();
		void		handleDelete();

	private:
		const HttpRequest*	_req;
		const Location*		_loc;
		int					_ncode;
		std::string			_body;
};

