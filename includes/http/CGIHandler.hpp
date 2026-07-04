#pragma once

#include "ARequestHandler.hpp"
#include "HttpRequest.hpp"
#include "../handlers/ClientHandler.hpp"
#include "../config/ConfigStruct.hpp"
#include "../cgi/CGIProcess.hpp"

class CGIHandler : public ARequestHandler {
	public:
		CGIHandler(const HttpRequest& req, std::string& path,
					std::string& interpreter,
					const std::string& peerAddr, IResponseSink* sink);
		~CGIHandler();

		std::string buildResponse();
		bool isAsync() const { return true; }

	private:
		const HttpRequest&	_req;
		std::string&		_path;
		std::string&		_interpreter;
		const std::string&	_peerAddr;
		CGIProcess*			_process;
};
