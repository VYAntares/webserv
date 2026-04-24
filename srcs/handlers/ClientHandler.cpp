#include "../../includes/handlers/ClientHandler.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

ClientHandler::ClientHandler(int clientFd, const Server& server) : _fd(clientFd), _server(server) {}

ClientHandler::~ClientHandler() { close(_fd); }

int ClientHandler::getFd() const { return _fd; }

void ClientHandler::handle_read() {
	char buf[4096];
	ssize_t n = recv(_fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0)
		return;
	buf[n] = '\0';
	std::cout << "=== REQUEST ===\n" << buf << "===============" << std::endl;
}
