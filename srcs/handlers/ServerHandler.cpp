#include "../../includes/handlers/ServerHandler.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <iostream>

ServerHandler::ServerHandler(addrport addrs, const Server& server) : _server(server) {
	try {
		_fd = createSocket();
		bindAddress(_fd, addrs);
		if (listen(_fd, SOMAXCONN) == -1)
			throw std::runtime_error("listen() failed:" + std::string(strerror(errno)));
	} catch (...) {
		close(_fd);
		throw;
	}
}

int ServerHandler::createSocket() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error("socket() failed:" + std::string(strerror(errno)));

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	return fd;
}

void ServerHandler::bindAddress(int serverFd, addrport listen) {
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = listen.first;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(listen.second);

	if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		throw std::runtime_error("bind() failed:" + std::string(strerror(errno)));

	std::cout << "bind() success" << std::endl;
}

// int ServerHandler::handle_accept() {

// }

int ServerHandler::getFd() const {
	return this->_fd;
}

ServerHandler::~ServerHandler() {
	close(_fd);
}
