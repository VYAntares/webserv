#pragma once

#include "../handlers/ServerHandler.hpp"
#include "../config/ConfigStruct.hpp"
#include "IEventHandler.hpp"
#include <sys/epoll.h>

#define MAX_EVENTS 128

class EventLoop {
	public:
		EventLoop(Config c);
		~EventLoop();
		// looping();
		
	private:
		int	_epfd;
};
