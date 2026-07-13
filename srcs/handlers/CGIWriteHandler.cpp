#include "../../includes/handlers/CGIWriteHandler.hpp"
#include <unistd.h>
#include <iostream>
#include <cerrno>

CGIWriteHandler::CGIWriteHandler(int fd, const std::string& body)
								: _fd(fd), _body(body), _sent(0) {
	std::cerr << "[DEBUG CGIWriteHandler ctor] fd=" << fd << " body.size()=" << body.size() << std::endl;
}



int CGIWriteHandler::handle_output() {
    if (_sent >= _body.size()) {         // body vide : rien à envoyer
			std::cerr << "[DEBUG CGIWriteHandler done] _sent=" << _sent << " body.size()=" << _body.size() << std::endl;
            return -1;
	}

    ssize_t n = write(_fd, _body.data() + _sent, _body.size() - _sent);
	std::cerr << "[DEBUG CGIWriteHandler write] n=" << n << " errno=" << errno << " _sent_before=" << _sent << std::endl;
    if (n <= 0)                         // le child a fermé son stdin → abandon
            return -1;

    _sent += static_cast<size_t>(n);
    return (_sent >= _body.size()) ? -1 : 0;
}



int	CGIWriteHandler::getFd() const { return _fd; }



CGIWriteHandler::~CGIWriteHandler() {
	std::cerr << "[DEBUG CGIWriteHandler dtor] fd=" << _fd << " _sent=" << _sent << " body.size()=" << _body.size() << std::endl;
	close(_fd);
}
