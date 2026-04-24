#pragma once

#include "../handlers/ServerHandler.hpp"
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

