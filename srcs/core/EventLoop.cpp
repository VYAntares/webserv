#include "../../includes/core/EventLoop.hpp"

EventLoop::EventLoop(ServerHandler toloopin) {
	_epfd = epoll_create(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create() failed: " + std::string(strerror(errno)));

}
