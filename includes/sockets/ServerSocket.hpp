#pragma once

#include "ASocket.hpp"
#include "../config/ConfigStruct.hpp"
#include <vector>
#include <iostream>
#include <cstring>
#include <fcntl.h>

class ServerSocket : public ASocket {
	public:
		ServerSocket(Config servers);
		~ServerSocket();
		int createSocket();
		int bindAddress(int serverFd,
						std::vector<addrport> listen);

		const int getFd() const;
		void onReadable();
		void onWritable();

	private:
		Config				_servers;
		std::vector<int>	_serverFd;
};

