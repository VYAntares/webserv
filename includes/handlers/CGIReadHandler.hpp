#pragma once

#include "../core/IEventHandler.hpp"
#include "../handlers/ClientHandler.hpp"

class CGIReadHandler : public IEventHandler {
	public:
		CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink);
		~CGIReadHandler() {}

		int getFd()		 const { return _fd; }
		int handle_input();

	private:
		int				_fd;
		pid_t			_pid;
		const Location*	_loc;
		IResponseSink*	_sink; // lie au client originel qui a lancer le cgi
};
