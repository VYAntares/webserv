#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

class ClientHandler : public IEventHandler {
	public:
		ClientHandler(int clientFd, const Server& server);
		~ClientHandler();
		int getFd() const;
		void handle_read();

	private:
		int		_fd;
		Server	_server;
};
