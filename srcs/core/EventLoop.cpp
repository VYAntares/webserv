#include "../../includes/core/EventLoop.hpp"
#include "../../includes/handlers/ServerHandler.hpp"

#include <iostream>
#include <string.h>

EventLoop::EventLoop(Config c) {
	std::vector<ServerHandler*> handlers;
	for (size_t i = 0; i < c.cfg.size(); i++) {
		for (size_t j = 0; j < c.cfg[i].listen.size(); j++) {
			std::cout << "server: " << i << std::endl;
			try {
				handlers.push_back(new ServerHandler(c.cfg[i].listen[j], c.cfg[i]));
			} catch (std::exception &e) {
				std::cerr << e.what() << std::endl;
			}
		}
	}

	_epfd = epoll_create(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create() failed: " + std::string(strerror(errno)));
	for (size_t i = 0; i < handlers.size(); i++) {
		struct epoll_event ev;
		ev.events = EPOLLIN;
		//ev.data.fd = handlers[i].
	}
}

EventLoop::~EventLoop() {
	for (size_t i = 0; i < handlers.size(); i++)
		delete handlers[i];
}
