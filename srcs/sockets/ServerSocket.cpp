#include "../../includes/sockets/ServerSocket.hpp"

ServerSocket::ServerSocket(Config servers) : _servers(servers) {
	try {
		for (size_t i = 0; i < _servers.srv.size(); i++) {
			std::cout << "server: " << i << std::endl;
			for (size_t j = 0; j < _servers.srv[i].listen.size(); j++) {
				std::cout << "listen: " << j << std::endl;
				int serverFd = createSocket();
				bindAddress(serverFd, _servers.srv[i].listen[j]);
				if (listen(serverFd, SOMAXCONN) == -1)
					throw std::runtime_error("socket() failed:" + std::string(strerror(errno)));
				_serverFd.push_back(serverFd);
			}
		}
	std::cout << "ca marche le constructeur" << std::endl;
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

int ServerSocket::createSocket() {
	int serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == -1) {
		throw std::runtime_error("socket() failed:" + std::string(strerror(errno)));
	}

	int opt = 1;
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(serverFd, F_SETFL, fcntl(serverFd, F_GETFL) | O_NONBLOCK);

	std::cout << "ca marche le socket" << std::endl;
	return serverFd;
}

void ServerSocket::bindAddress(int serverFd, addrport listen) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = listen.first;
	addr.sin_family = AF_INET;
	addr.sin_port = listen.second;

	if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		throw std::runtime_error("bind() failed:" + std::string(strerror(errno)));
	std::cout << "ca marche le bind" << std::endl;
}
