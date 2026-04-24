#include "../../includes/core/EventLoop.hpp"
#include "../../includes/handlers/ServerHandler.hpp"

#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>

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

void EventLoop::addHandler(IEventHandler* h, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = h;
	epoll_ctl(_epfd, EPOLL_CTL_ADD, h->getFd(), &ev);
} 

void EventLoop::looping() {
	std::cout << "Server available at: http://localhost:" << std::endl;
	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int n = epoll_wait(_epfd, events, MAX_EVENTS, 3000);
		for (int i = 0; i < n; i++) {
			IEventHandler* h = static_cast<IEventHandler*>(events[i].data.ptr);
			if (events[i].events & EPOLLIN) {
				IEventHandler* newHandler = static_cast<IEventHandler*>(h->handle_accept());
				if (newHandler)
					addHandler(newHandler, EPOLLIN);
				else
					h->handle_read();
			}
		}
	}
}

EventLoop::~EventLoop() {
	for (size_t i = 0; i < _handlers.size(); i++)
		delete _handlers[i];
}
