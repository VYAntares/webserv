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



// Initialise le socket serveur puis s'enregistre auprès du singleton EventLoop
// pour recevoir les événements ACCEPT. En cas d'échec, ferme le fd (RAII).
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

// Crée un socket TCP non bloquant.
// SO_REUSEADDR évite "Address already in use" lors d'un redémarrage rapide du serveur.
// O_NONBLOCK est requis pour que la boucle événementielle ne se bloque pas sur accept().
int ServerHandler::createSocket() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error("socket() failed:" + std::string(strerror(errno)));

	// fermer fd avant de throw : à ce stade _fd n'est pas encore assigné,
	// le catch du constructeur ne peut donc pas le fermer pour nous.
	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		close(fd);
   		throw std::runtime_error("setsockopt() failed: " + std::string(strerror(errno)));
	}
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1) {
		close(fd);
		throw std::runtime_error("fcntl() failed: " + std::string(strerror(errno)));
	}
	// l'enfant CGI ne doit pas hériter du socket d'écoute
	fcntl(fd, F_SETFL, FD_CLOEXEC);
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

// Accepte une connexion entrante et crée un ClientHandler dynamique.
// Le ClientHandler s'enregistre lui-même dans l'EventLoop via son constructeur.
// Retourne toujours 0 : un échec d'accept() (connexion annulée, plus de fd
// disponibles...) est transitoire et ne doit surtout pas détruire le socket
// d'écoute — retourner -1 ici ferait supprimer ce ServerHandler par EventLoop
// et le port ne répondrait plus jamais.
int ServerHandler::handle_accept() {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(_fd, (struct sockaddr*)&client_addr, &len);
	if (client_fd == -1)
		return 0;

	// Le fd client n'hérite PAS de O_NONBLOCK du socket d'écoute (Linux) :
	// il faut le poser explicitement, sinon un send() vers un client lent
	// peut bloquer toute la boucle (interdit par le sujet).
	if (fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK) == -1) {
		close(client_fd);
		return 0;
	}
	// FD_CLOEXEC : si un CGI est fork+execvé plus tard, le kernel
	// ferme ce fd automatiquement dans l'enfant pendant l'execve().
	// Le parent continue de l'utiliser normalement (keep-alive, etc.).
	fcntl(client_fd, F_SETFL, FD_CLOEXEC);
	
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

