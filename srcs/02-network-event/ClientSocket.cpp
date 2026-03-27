#pragma once

#include "includes/network/ASocket.hpp"

ClientSocket::ClientSocket(int fd) : ASocket(fd) {}

ClientSocket::~ClientSocket() {
	if (fd >= 0) {
		close(_fd);
		_fd = -1;
	}
}

void ClientSocket::handleError() {
	
}