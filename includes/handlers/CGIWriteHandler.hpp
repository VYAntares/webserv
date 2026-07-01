#pragma once

#include "../core/IEventHandler.hpp"

class CGIWriteHandler : public IEventHandler {
	public:
		CGIWriteHandler(int fd) : _fd(fd) {}
		~CGIWriteHandler() {}

		int getFd()		  const { return _fd; }
		int handle_output()	  { return 0; }

	private:
		int _fd;
};
