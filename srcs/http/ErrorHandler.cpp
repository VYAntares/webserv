#include "../../includes/http/ErrorHandler.hpp"
#include <iostream>

ErrorHandler::ErrorHandler(const Location& loc, int code) {
	_ncode = code;
	if (loc.return_path.first != -1) { 
		handleReturn(loc.return_path);
		return ;
	}
	setErrorPage(loc);
}



ErrorHandler::ErrorHandler(const Location& loc, int code, std::string& body) {
	_ncode = code;
	_body = body;

	if (loc.return_path.first != -1) { 
		handleReturn(loc.return_path);
		return ;
	}
}



ErrorHandler::ErrorHandler(const Server& server, int code) {
	_ncode = code;
	if (server.return_path.first != -1) {
		handleReturn(server.return_path);
		return ;
	}
	std::map<int, std::string>::const_iterator it = server.error_page.find(_ncode);

	if (it != server.error_page.end())
		_errorpage = it->second;
}

