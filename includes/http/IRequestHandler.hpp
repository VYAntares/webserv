#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include "../utils/utils.hpp"

class IRequestHandler {
	public:
		virtual				~IRequestHandler() {}
		std::string			buildResponse();
		void				getErrorPage();
		void 				handleReturn(const std::pair<int, std::string>& return_path);

	protected:
		int					_ncode;
		std::string			_type;
		std::string			_body;
		std::string			_location;
		std::string			_errorpage;
};
