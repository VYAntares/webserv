#pragma once

#include "../handlers/ServerHandler.hpp"
#include "../config/ConfigStruct.hpp"
#include "IEventHandler.hpp"
#include <vector>
#include <sys/epoll.h>

#define MAX_EVENTS 128

class EventLoop {
	public:
		EventLoop(Config c);
		~EventLoop();
		void addHandler(IEventHandler* h, uint32_t events);
		void looping();
		
	private:
		int							_epfd;
		std::vector<IEventHandler*>	_handlers;
};

