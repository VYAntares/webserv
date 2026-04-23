#pragma once

#include "../handlers/AcceptHandler.hpp"
#include "IEventHandler.hpp"
#include <sys/epoll.h>

#define MAX_EVENTS 128

class EventLoop.h {
	public:
		EventLoop();
		~EventLoop();
		looping();
	private:
		int	_epfd;
}
