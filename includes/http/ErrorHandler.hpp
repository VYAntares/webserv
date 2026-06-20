#pragma once

#include "../config/ConfigStruct.hpp"
#include "IRequestHandler.hpp"
#include "../utils/utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

class ErrorHandler : public IRequestHandler {
	public:
		ErrorHandler(const Server& server, int code);
		ErrorHandler(const Location& loc, int code);
		~ErrorHandler() {};
		std::string	buildResponse();
		void		getErrorPage();
		void 		handleReturn(const std::pair<int, str>& return_path);
	
	private:
		int			_error;
		std::string	_errorpage;
		std::string	_body;
		std::string	_type;
		std::string	_location;
};

