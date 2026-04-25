#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

// Représente un socket serveur en écoute sur une paire (IP, port).
// Hérite de IEventHandler pour s'intégrer dans la boucle événementielle.
// handle_accept() crée et retourne un ClientHandler à chaque nouvelle connexion.
class ServerHandler : public IEventHandler {
	public:
		ServerHandler(addrport listen, const Server& server);
		~ServerHandler();

		// IEventHandler
		int		getFd() const;
		int		handle_accept();

		int		createSocket();
		void	bindAddress(int serverFd, addrport listen);

	private:
		Server	_server;
		int		_fd;
};
