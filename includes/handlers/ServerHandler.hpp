#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

class ServerHandler : public IEventHandler {
	public:
		ServerHandler(addrport listen, const Server& server);
		~ServerHandler();

		// IEventHandler
		int getFd() const;
		IEventHandler* handle_accept();

		int createSocket();
		void bindAddress(int serverFd, addrport listen);


	private:
		Server	_server;
		int		_fd;
};
