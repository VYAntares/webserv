#pragma once

#include "../config/ConfigStruct.hpp"
#include "IEventHandler.hpp"
#include <vector>
#include <sys/epoll.h>

#define MAX_EVENTS 128

// Implémente le Reactor Pattern : initialise les ServerHandlers à partir
// de la config, les enregistre dans epoll, puis dispatche chaque événement
// vers le bon handler via looping().
// _epfd     : fd de l'instance epoll partagée par tous les handlers.
// _handlers : liste des ServerHandlers (ownership — détruits dans le destructeur).

struct HandlerEntry {
	IEventHandler*	handler;
	EventType		type;
};

class EventLoop {
	public:
		static EventLoop* instance();

		void register_handler(IEventHandler* h, EventType type);
		void remove_handler(IEventHandler* h, EventType type);
		void handle_events();
		
	private:
		EventLoop();
		~EventLoop();

		int							_epfd;
		std::vector<HandlerEntry*>	_table;

		static EventLoop* 			_instance;
};

