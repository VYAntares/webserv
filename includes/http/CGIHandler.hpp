#pragma once

#include "ARequestHandler.hpp"
#include "HttpRequest.hpp"
#include "../config/ConfigStruct.hpp"
#include "../cgi/CGIProcess.hpp"

class CGIHandler : public ARequestHandler {
	public:
		CGIHandler(const HttpRequest& req, const Location* loc,
					std::string& path, std::string& interpreter,
					const std::string& peerAddr);
		~CGIHandler();
	
		std::string buildResponse();
	
	private:
		const HttpRequest&	_req;
		const Location*		_loc;
		std::string&		_path;
		std::string&		_interpreter;
		const std::string&	_peerAddr;
		CGIProcess			_process;
};
