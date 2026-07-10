#pragma once

#include "../http/HttpRequest.hpp"
#include "../handlers/ClientHandler.hpp"
#include "../cgi/CGIProcess.hpp"

class CGIHandler {
	public:
		CGIHandler(const HttpRequest& req, std::string& path,
					std::string& interpreter,
					const std::string& peerAddr,
					const Location* loc,
					IResponseSink* sink);
		~CGIHandler();

	private:
		CGIProcess*			_process;
};

