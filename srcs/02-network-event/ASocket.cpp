#include "../../includes/02-network-event/ASocket.hpp"

ASocket::ASocket(int fd) : _fd(fd),
	  					   _lastActivity(std::time(NULL)),
						   _finished(false) {}

ASocket::~ASocket() {}

int ASocket::getFd() const {
	return this->_fd;
}

void ASocket::updateActivity() {
	this->_lastActivity = std::time(NULL);
}

void ASocket::markFinished() {
	this->_finished = true;
}