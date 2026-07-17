#include "../../includes/core/EventLoop.hpp"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#include <map>

static const int CLIENT_TIMEOUT = 30;

static volatile sig_atomic_t g_stop = 0;

static void handle_sigint(int) { g_stop = 1; }

static uint32_t to_epoll_flags(EventType type) {
	uint32_t flags = 0;
	if (type & (ACCEPT_EVENT | READ_EVENT))	flags |= EPOLLIN;
	if (type & WRITE_EVENT)					flags |= EPOLLOUT;
	return flags;
}

EventLoop* EventLoop::_instance = NULL;

EventLoop* EventLoop::instance() {
	if (!_instance)
		_instance = new EventLoop();
	return _instance;
}

EventLoop::EventLoop() {
	_epfd = epoll_create1(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create() failed: " + std::string(strerror(errno)));
	fcntl(_epfd, F_SETFD, FD_CLOEXEC);
}

void EventLoop::register_handler(IEventHandler* h, EventType type) {
	HandlerEntry* entry	= new HandlerEntry();
	entry->handler		= h;
	entry->type			= type;
	_table[h]			= entry;

	struct epoll_event ev;
	ev.events	= to_epoll_flags(type);
	ev.data.ptr	= entry;
	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, h->getFd(), &ev) == -1) {
		_table.erase(h);
		delete entry;
		throw std::runtime_error("epoll_ctl(ADD) failed: " + std::string(strerror(errno)));
	}
}

void EventLoop::modify_handler(IEventHandler* h, EventType newType) {
	std::map<IEventHandler*, HandlerEntry*>::iterator it = _table.find(h);
	if (it == _table.end())
		return;

	it->second->type = newType;

	struct epoll_event ev;
	ev.events	= to_epoll_flags(newType);
	ev.data.ptr	= it->second;
	if (epoll_ctl(_epfd, EPOLL_CTL_MOD, h->getFd(), &ev) == -1)
		std::cerr << "epoll_ctl(MOD) failed fd=" << h->getFd() << ": " << strerror(errno) << "\n";
}

void EventLoop::remove_handler(IEventHandler* h) {
	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, h->getFd(), NULL) == -1)
		std::cerr << "epoll_ctl(DEL) failed fd=" << h->getFd() << ": " << strerror(errno) << "\n";

	std::map<IEventHandler*, HandlerEntry*>::iterator it = _table.find(h);
	if (it != _table.end()) {
		delete it->second;
		_table.erase(it);
	}
}

void EventLoop::handle_events() {
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);
	signal(SIGQUIT, handle_sigint);
	signal(SIGPIPE, SIG_IGN);

	std::cout << "Server listening.." << std::endl;
	struct epoll_event events[MAX_EVENTS];

	while (!g_stop) {
		int n = epoll_wait(_epfd, events, MAX_EVENTS, 500);

		for (int i = 0; i < n; i++) {
			HandlerEntry* entry	= static_cast<HandlerEntry*>(events[i].data.ptr);
			IEventHandler* h	= entry->handler;
			int ret = 0;

			try {
				if (entry->type == ACCEPT_EVENT)
					ret = h->handle_accept();
				else {
					if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR))
						ret = h->handle_input();
					if (ret != -1 && events[i].events & EPOLLOUT)
						ret = h->handle_output();
					if (ret != -1 && (events[i].events & (EPOLLHUP | EPOLLERR))
						&& !(events[i].events & (EPOLLIN | EPOLLOUT)))
						ret = -1;
				}
			} catch (std::exception& e) {
				std::cerr << "[handler error] fd=" << h->getFd() << ": " << e.what() << "\n";
				ret = -1;
			} catch (...) {
				std::cerr << "[handler error] fd=" << h->getFd() << ": unknown exception\n";
				ret = -1;
			}

			if (ret == -1) {
				remove_handler(h);
				delete h;
			}
		}
		checkTimeOut();
	}
}

void EventLoop::checkTimeOut() {
	time_t now = time(NULL);

	std::vector<IEventHandler*> timedOut;
	for (std::map<IEventHandler*, HandlerEntry*>::iterator it = _table.begin();
			it != _table.end(); ++it) {
		time_t last = it->first->getLastActivity();
		if (last != 0 && (now - last) > CLIENT_TIMEOUT)
			timedOut.push_back(it->first);
	}

	for (size_t i = 0; i < timedOut.size(); i++) {
		std::cout << "[timeout] fd=" << timedOut[i]->getFd() << " closed after "
		          << CLIENT_TIMEOUT << "s of inactivity\n";
		if (timedOut[i]->handle_timeout())
			continue;
		remove_handler(timedOut[i]);
		delete timedOut[i];
	}
}

void EventLoop::destroy() { delete _instance; }

EventLoop::~EventLoop() {
	for (std::map<IEventHandler*, HandlerEntry*>::iterator it = _table.begin();
			it != _table.end(); ++it) {
		delete it->first;
		delete it->second;
	}
	close(_epfd);
	_instance = NULL;
	std::cout << "\nServer closing..." << std::endl;
	std::cout << "cya babyyyy" << std::endl;
}

