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

// Initialise le socket serveur en enchaînant createSocket(), bindAddress() et listen().
// En cas d'échec, ferme le fd avant de relancer l'exception (RAII).
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
	EventLoop::instance()->register_handler(this, ACCEPT_EVENT);
}

// Crée un socket TCP non bloquant.
// SO_REUSEADDR évite "Address already in use" lors d'un redémarrage rapide du serveur.
// O_NONBLOCK est requis pour que la boucle événementielle ne se bloque pas sur accept().
int ServerHandler::createSocket() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error("socket() failed:" + std::string(strerror(errno)));

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	return fd;
}

// Associe l'adresse IP et le port fournis au fd du socket.
// htons() convertit le port en ordre réseau (big-endian).
void ServerHandler::bindAddress(int serverFd, addrport listen) {
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = listen.first;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(listen.second);

	if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		throw std::runtime_error("bind() failed:" + std::string(strerror(errno)));
}

// Accepte une connexion entrante et retourne un ClientHandler alloué dynamiquement.
// Le retour en IEventHandler* permet à la boucle événementielle de traiter
// tous les handlers de façon uniforme via le polymorphisme.
int ServerHandler::handle_accept() {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(_fd, (struct sockaddr*)&client_addr, &len);
	if (client_fd == -1)
		return -1;
	new ClientHandler(client_fd, _server);
	return 0;
}

int ServerHandler::getFd() const { return this->_fd; }

ServerHandler::~ServerHandler() { close(_fd); }
