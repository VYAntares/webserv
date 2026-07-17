#pragma once

#include "../core/IEventHandler.hpp"
#include "../handlers/ClientHandler.hpp"

struct output {
	int			status;
	std::string	contentT;
	std::string	header;
	std::string body;

	output() : status(200), contentT(""), header(""), body("") {}
};

class CGIReadHandler : public IEventHandler {
	public:
		CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink);
		~CGIReadHandler();

		int 			getFd()		 const { return _fd; }
		int 			handle_input();
		time_t			getLastActivity() const { return _lastActivity; }

		void			detachSink() { _sink = NULL; }
		void			reapChild(int& status);
		std::string		parseCgi();

	private:
		int				_fd;
		pid_t			_pid;
		std::string		_out;
		const Location*	_loc;
		IResponseSink*	_sink;
		time_t			_lastActivity;
		bool			_reaped;
		bool			_responded;
};

