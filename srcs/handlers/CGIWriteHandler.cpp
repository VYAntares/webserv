#include "../../includes/handlers/CGIWriteHandler.hpp"
#include <unistd.h>



CGIWriteHandler::CGIWriteHandler(int fd, const std::string& body)
								: _fd(fd), _body(body), _sent(0) {}



int CGIWriteHandler::handle_output() {
    if (_sent >= _body.size())          // body vide : rien à envoyer
            return -1;

    ssize_t n = write(_fd, _body.data() + _sent, _body.size() - _sent);
    if (n <= 0)                         // le child a fermé son stdin → abandon
            return -1;

    _sent += static_cast<size_t>(n);
    return (_sent >= _body.size()) ? -1 : 0;
}



int	CGIWriteHandler::getFd() const { return _fd; }



CGIWriteHandler::~CGIWriteHandler() { close(_fd); }
