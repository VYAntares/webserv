#pragma once

#include <string>

class CGIReadHandler;

class IResponseSink {
	public:
		virtual void onCgiDone(const std::string& rawHttpResp) = 0;
		virtual void onCgiStart(CGIReadHandler* rd) { (void)rd; }
		virtual ~IResponseSink() {}
};

