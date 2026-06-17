#pragma once

#include "IRequestHandler.hpp"
#include "HttpRequest.hpp"
#include "../config/ConfigStruct.hpp"
#include "../utils/vectors.hpp"
#include <string>
#include <iostream>
#include <map>

class StaticHandler : public IRequestHandler {
	public:
		StaticHandler(const HttpRequest& req, const Location& loc, const std::string& path);
		~StaticHandler();
		std::string buildResponse();
		std::string	getReason();
		std::string	getType(const std::string& path);
		void		handleGet();
		void		handlePost();
		void		handleDelete();
		void		handleReturn();

	private:
		const HttpRequest*	_req;
		const Location*		_loc;
		int					_ncode;
		std::string			_type;
		std::string			_body;
		std::string			_path;
		std::string			_location;
};

