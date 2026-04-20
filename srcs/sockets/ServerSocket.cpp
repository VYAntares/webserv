#include "../../includes/sockets/ServerSocket.hpp"

ServerSocket::ServerSocket(Config servers) : _servers(servers) {
	try {
		for (size_t i = 0; i < _servers.srv.size(); i++) {
			_serverFd.push_back(createSocket());
			for (size_t j = 0; j < _servers.srv[i].listen.size(); j++)
				bindAddress(_serverFd[i], _servers.srv[i].listen);
		}
	} catch (const std::exception &e) {

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

	return serverFd;
}

int ServerSocket::bindAddress(int serverFd, std::vector<addrport> listen) {

}
