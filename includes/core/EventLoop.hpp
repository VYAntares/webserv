#pragma once

#include "../config/ConfigStruct.hpp"
#include "IEventHandler.hpp"
#include <map>
#include <sys/epoll.h>

#define MAX_EVENTS 128

struct HandlerEntry {
	IEventHandler*	handler;
	EventType		type;
};

class EventLoop {
	public:
		static EventLoop* instance();
		static void destroy();

		void register_handler(IEventHandler* h, EventType type);
		void modify_handler(IEventHandler* h, EventType newType);
		void remove_handler(IEventHandler* h);
		void handle_events();
		void checkTimeOut();

	private:
		EventLoop();
		~EventLoop();

		int										_epfd;

		std::map<IEventHandler*, HandlerEntry*>	_table;

		static EventLoop* 						_instance;
};

