#pragma once

#include "../config/ConfigStruct.hpp"
#include "IRequestHandler.hpp"
#include "../utils/utils.hpp"
#include "HttpRequest.hpp"
#include <unistd.h> 
#include <iostream>
#include <fstream>

class StaticHandler : public IRequestHandler {
	public:
		StaticHandler(const HttpRequest& req, const Location& loc, const std::string& path);
		~StaticHandler() {};
		std::string buildResponse();
		void		handleGet();
		void		handlePost();
		void		handleDelete();
		void		handleReturn();
		void		throwList();
		void		headerListe(const std::string& path);

	private:
		const HttpRequest*	_req;
		const Location*		_loc;
		int					_ncode;
		std::string			_type;
		std::string			_body;
		std::string			_path;
		std::string			_location;
};
