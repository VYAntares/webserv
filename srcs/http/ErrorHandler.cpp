#include "../../includes/http/ErrorHandler.hpp"
#include <iostream>

ErrorHandler::ErrorHandler(const BaseBlock& b, int code, std::string body) {
	_ncode = code;
	if (!body.empty())
		_body = body;
	if (b.return_path.first != -1) { 
		handleReturn(b.return_path);
		return ;
	}
	setErrorPage(b);
}
