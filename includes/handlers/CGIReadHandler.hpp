#pragma once

#include "../core/IEventHandler.hpp"

class CGIReadHandler : public IEventHandler {
	public:
		CGIReadHandler(int fd) : _fd(fd) {}
		~CGIReadHandler() {}

		int getFd()		 const { return _fd; }
		int handle_input()	 { return 0; }

	private:
		int _fd;
};
