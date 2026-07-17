#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

class ServerHandler : public IEventHandler {
	public:
		ServerHandler(addrport listen, const Server& server);
		~ServerHandler();

		int		getFd() const;
		int		handle_accept();

		int		createSocket();
		void	bindAddress(int serverFd, addrport listen);
		void	printServer() const;

	private:
		Server	_server;
		int		_fd;
};

