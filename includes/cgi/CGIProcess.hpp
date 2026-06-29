#pragma once

#include "../config/ConfigStruct.hpp"
#include "../http/HttpRequest.hpp"

class CGIProcess {
	public:
		CGIProcess(const HttpRequest& req, const Location* loc,
				std::string& path, std::string& interpreter,
				const std::string& peerAddr);
		~CGIProcess();
};
