#pragma once

#include "../core/IEventHandler.hpp"
#include <iostream>

class CGIWriteHandler : public IEventHandler {
	public:
		CGIWriteHandler(int fd, const std::string& body) : _fd(fd), _body(body) {}
		~CGIWriteHandler() {}

		int getFd()		  const { return _fd; }
		int handle_output();

	private:
		int 				_fd;
		const std::string	_body;
};
