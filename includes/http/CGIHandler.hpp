#pragma once

#include "HttpRequest.hpp"
#include "../handlers/ClientHandler.hpp"
#include "../cgi/CGIProcess.hpp"

class CGIHandler {
	public:
		CGIHandler(const HttpRequest& req, std::string& path,
					std::string& interpreter,
					const std::string& peerAddr, IResponseSink* sink);
		~CGIHandler();

	private:
		const HttpRequest&	_req;
		std::string&		_path;
		std::string&		_interpreter;
		const std::string&	_peerAddr;
		CGIProcess*			_process;
};
