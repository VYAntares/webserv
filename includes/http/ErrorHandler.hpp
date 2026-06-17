#pragma once

#include "IRequestHandler.hpp"
#include "../config/ConfigStruct.hpp"
#include "../utils/vectors.hpp"
#include <string>
#include <iostream>
#include <sstream>

class ErrorHandler : public IRequestHandler {
	public:
		ErrorHandler(const Server& server, int code);
		ErrorHandler(const Location& loc, int code);
		~ErrorHandler();
		std::string	buildResponse();
		std::string	getReason();
		std::string	getType(const std::string& path);
		void		getErrorPage();
		void 		handleReturn(const std::pair<int, str>& return_path);
	
	private:
		int			_error;
		std::string	_errorpage;
		std::string	_body;
		std::string	_type;
		std::string	_location;
};

