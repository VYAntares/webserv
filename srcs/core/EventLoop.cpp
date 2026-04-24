#include "../../includes/core/EventLoop.hpp"
#include "../../includes/handlers/ServerHandler.hpp"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

// Crée l'instance epoll unique qui surveillera tous les fd de la boucle.
// Pour chaque adresse écoutée dans la config, instancie un ServerHandler
// et l'enregistre dans epoll via addHandler().
// Le Reactor Pattern permet ensuite de dispatcher les événements sans manipuler les fd directement.
EventLoop::EventLoop(Config c) {
	_epfd = epoll_create1(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create() failed: " + std::string(strerror(errno)));
	for (size_t i = 0; i < c.cfg.size(); i++) {
		for (size_t j = 0; j < c.cfg[i].listen.size(); j++) {
			try {
				IEventHandler* h = new ServerHandler(c.cfg[i].listen[j], c.cfg[i]);
				_handlers.push_back(h);
				addHandler(h, EPOLLIN);
			} catch (std::exception &e) {
				std::cerr << e.what() << std::endl;
			}
		}
	}
}

// Boucle événementielle principale (Reactor Pattern).
// epoll_wait() bloque jusqu'à 3s en attendant un événement.
// Pour chaque EPOLLIN : si handle_accept() retourne un nouveau handler (connexion serveur),
// on l'ajoute à epoll ; sinon on délègue à handle_read() (client ou CGI).
void EventLoop::looping() {
    signal(SIGPIPE, SIG_IGN);
	std::cout << "Server available at: http://localhost:" << std::endl;
	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int n = epoll_wait(_epfd, events, MAX_EVENTS, -1);
		for (int i = 0; i < n; i++) {
			IEventHandler* h = static_cast<IEventHandler*>(events[i].data.ptr);
			if (events[i].events & EPOLLIN) {
				IEventHandler* newHandler = (h->handle_accept());
				if (newHandler)
					addHandler(newHandler, EPOLLIN);
				else
					h->handle_read();
			}
		}
	}
}

// Enregistre un handler dans epoll en stockant son pointeur dans data.ptr.
// C'est ce pointeur qui permet le dispatch polymorphique dans looping()
// sans avoir besoin de table de correspondance fd → handler.
void EventLoop::addHandler(IEventHandler* h, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = h;
	epoll_ctl(_epfd, EPOLL_CTL_ADD, h->getFd(), &ev);
}

EventLoop::~EventLoop() {
	for (size_t i = 0; i < _handlers.size(); i++)
		delete _handlers[i];
}
