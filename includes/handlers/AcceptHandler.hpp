#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

class AcceptHandler : public IEventHandler {
	public:
		AcceptHandler(addrport listen, const Server& server);
		~AcceptHandler();

		// IEventHandler
		int getFd() const;
		// void handle_accept();

		int createSocket();
		void bindAddress(int serverFd, addrport listen);


	private:
		Server	_server;
		int		_fd;
};
