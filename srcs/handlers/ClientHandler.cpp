#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

ClientHandler::ClientHandler(int clientFd, const Server& server)
							 : _fd(clientFd), _server(server) {
	EventLoop::instance()->register_handler(this, READ_EVENT);							
}

ClientHandler::~ClientHandler() { close(_fd); }

int ClientHandler::getFd() const { return _fd; }

int ClientHandler::handle_input() {
	char buf[4096];
	ssize_t n = recv(_fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0)
		return -1;
	buf[n] = '\0';
	std::cout << "=== REQUEST ===\n" << buf << "===============" << std::endl;
	return 0;
}
