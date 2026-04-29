#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

class ClientHandler : public IEventHandler {
	public:
		ClientHandler(int clientFd, const Server& server);
		~ClientHandler();
		
		int getFd() const;
		int handle_input();
		int handle_output();

	private:
		int		_fd;
		size_t	_sent;
		Server	_server;
};
