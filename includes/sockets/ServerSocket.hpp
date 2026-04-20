#pragma once

#include "ASocket.hpp"
#include "../config/ConfigStruct.hpp"
#include <vector>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <arpa/inet.h>

class ServerSocket : public ASocket {
	public:
		ServerSocket(Config servers);
		~ServerSocket() {}
		int createSocket();
		void bindAddress(int serverFd,
						addrport listen);

		void onReadable();
		void onWritable();

	private:
		Config				_servers;
		std::vector<int>	_serverFd;
};

