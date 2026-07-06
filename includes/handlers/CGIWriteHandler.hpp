#pragma once

#include "../core/IEventHandler.hpp"
#include <string>

class CGIWriteHandler : public IEventHandler {
	public:
		CGIWriteHandler(int fd, const std::string& body);
		~CGIWriteHandler();

		int getFd()	const;
		int handle_output();

	private:
		int 				_fd;
		const std::string	_body;
		size_t				_sent;
};
