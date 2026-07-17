#pragma once

#include <cstddef>
#include <ctime>

enum EventType {
	ACCEPT_EVENT	= 01,
	READ_EVENT		= 02,
	WRITE_EVENT		= 04,
	TIMEOUT_EVENT	= 010,
	SIGNAL_EVENT	= 020,
	CLOSE_EVENT		= 040
};

class IEventHandler {
	public:
		virtual int		getFd()				const = 0;
		virtual int		handle_accept()		{ return 0; }
		virtual int		handle_input()		{ return 0; }
		virtual int		handle_output()		{ return 0; }
		virtual int		handle_close()		{ return 0; }
		virtual int		handle_timeout()	{ return 0; }
		virtual time_t	getLastActivity()	const { return 0; }
		virtual			~IEventHandler()	{}
};

