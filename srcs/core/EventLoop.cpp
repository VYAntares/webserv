#include "../../includes/core/EventLoop.hpp"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

EventLoop* EventLoop::_instance = NULL;

EventLoop* EventLoop::instance() {
	if (!_instance)
		_instance = new EventLoop();
	return _instance;
}

// Crée l'instance epoll unique qui surveillera tous les fd de la boucle.
EventLoop::EventLoop() {
	_epfd = epoll_create1(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create() failed: " + std::string(strerror(errno)));
}

void EventLoop::register_handler(IEventHandler* h, EventType type) {
	HandlerEntry* entry	= new HandlerEntry();
	entry->handler		= h;
	entry->type			= type;
	_table.push_back(entry);

	struct epoll_event ev;
	ev.events	= EPOLLIN;
	ev.data.ptr	= entry;
	epoll_ctl(_epfd, EPOLL_CTL_ADD, h->getFd(), &ev);
}

void EventLoop::remove_handler(IEventHandler* h, EventType type) {
	(void)type;
	epoll_ctl(_epfd, EPOLL_CTL_DEL, h->getFd(), NULL);
	for (size_t i = 0; i < _table.size(); i++) {
		if (_table[i]->handler == h) {
			delete _table[i];
			_table.erase(_table.begin() + i);
			break;
		}
	}
}

void EventLoop::handle_events() {
	std::cout << "Server listening.." << std::endl;
	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int n = epoll_wait(_epfd, events, MAX_EVENTS, -1);

		for (int i = 0; i < n; i++) {

			HandlerEntry* entry	= static_cast<HandlerEntry*>(events[i].data.ptr);
			IEventHandler* h	= entry->handler;

			if (entry->type == ACCEPT_EVENT) {
				if (h->handle_accept() == -1) {
					EventLoop::instance()->remove_handler(h, ACCEPT_EVENT);
					delete h;
				}
			}
			else if (entry->type == READ_EVENT) {
				if (h->handle_input() == -1) {
					EventLoop::instance()->remove_handler(h, READ_EVENT);
					delete h;
				}
			}
		}
	}
}

EventLoop::~EventLoop() {
	for (size_t i = 0; i < _table.size(); i++) {
		delete _table[i]->handler;
		delete _table[i];
	}
}
