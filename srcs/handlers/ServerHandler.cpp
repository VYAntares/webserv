#include "../../includes/handlers/ServerHandler.hpp"
#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <iostream>

// static std::string	sizeToStr(size_t n) {
// 	std::ostringstream oss;
// 	oss << n;
// 	return oss.str();
// }

// static void	printBase(const BaseBlock& b, const std::string& pad) {
// 	if (!b.root.empty())
// 		std::cout << pad << "root            : " << b.root << "\n";
// 	if (!b.index.empty())
// 		std::cout << pad << "index           : " << b.index << "\n";
// 	if (b.autoindex != -1)
// 		std::cout << pad << "autoindex       : " << (b.autoindex ? "on" : "off") << "\n";
// 	if (b.max_body_client != SIZE_MAX)
// 		std::cout << pad << "max_body_client : " << sizeToStr(b.max_body_client) << " bytes\n";
// 	if (b.return_path.first != -1)
// 		std::cout << pad << "return          : " << b.return_path.first << " " << b.return_path.second << "\n";
// 	for (std::map<int, str>::const_iterator it = b.error_page.begin(); it != b.error_page.end(); ++it)
// 		std::cout << pad << "error_page      : " << it->first << " -> " << it->second << "\n";
// }

void	ServerHandler::printServer() const {
	for (size_t i = 0; i < _server.server_name.size(); i++)
		std::cout << "│  server_name   : " << _server.server_name[i] << "\n";
	for (size_t i = 0; i < _server.locations.size(); i++)
		std::cout << "│  location      : " << _server.locations[i].path << "\n";
	std::cout << "└─\n";
}

ServerHandler::ServerHandler(addrport addrs, const Server& server) : _server(server), _fd(-1) {
	try {
		_fd = createSocket();
		std::cout << "┌─ [SERVER fd=" << _fd << "] addr: " << addrToStr(addrs.first) << ":" << addrs.second << std::endl;
		bindAddress(_fd, addrs);
		if (listen(_fd, SOMAXCONN) == -1)
			throw std::runtime_error("listen() failed:" + std::string(strerror(errno)));
	} catch (...) {
		close(_fd);
		throw;
	}
	EventLoop::instance()->register_handler(this, ACCEPT_EVENT);
	printServer();
}

int ServerHandler::createSocket() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error("socket() failed:" + std::string(strerror(errno)));

	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		close(fd);
   		throw std::runtime_error("setsockopt() failed: " + std::string(strerror(errno)));
	}
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1) {
		close(fd);
		throw std::runtime_error("fcntl() failed: " + std::string(strerror(errno)));
	}
	fcntl(fd, F_SETFD, FD_CLOEXEC);
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
}

int ServerHandler::handle_accept() {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(_fd, (struct sockaddr*)&client_addr, &len);
	if (client_fd == -1)
		return 0;

	if (fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK) == -1) {
		close(client_fd);
		return 0;
	}
	fcntl(client_fd, F_SETFD, FD_CLOEXEC);

	try {
		new ClientHandler(client_fd, _server, client_addr);
	} catch (std::exception& e) {
		std::cerr << "accept: " << e.what() << std::endl;
		close(client_fd);
	}
	return 0;
}

int ServerHandler::getFd() const { return this->_fd; }

ServerHandler::~ServerHandler() { close(_fd); }

