#include "../../includes/handlers/CGIReadHandler.hpp"
#include "../../includes/http/ErrorHandler.hpp"
#include "../../includes/http/StaticHandler.hpp"
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <sys/wait.h>



CGIReadHandler::CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink) 
							: _fd(fd), _pid(pid), _loc(loc), _sink(sink) {
}



int CGIReadHandler::handle_input() {
	char buf[4096];
	ssize_t n = read(_fd, buf, sizeof(buf));

	if (n > 0) {
		_out.append(buf, n);
		return 0;								// pas fini : epoll nous rappellera
	}
	if (n < 0) {								// erreur réelle (epoll a dit "lisible")
		ErrorHandler err(*_loc, 502);
		_sink->onCgiDone(err.buildResponse());	// ne pas laisser le client sans réponse
		return -1;
	}

	// n == 0 : EOF — le CGI a fermé son stdout, _out est complet
	int status = 0;
	waitpid(_pid, &status, 0);

	ARequestHandler* a = NULL;
	if (WIFSIGNALED(status))
			a = new ErrorHandler(*_loc, 502);
	else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			a = new ErrorHandler(*_loc, 500, _out);
	else
			a = new StaticHandler(*_loc, 200, _out);

	std::string raw = a->buildResponse();
	delete a;
	_sink->onCgiDone(raw);
	return -1;									// EventLoop nous retire et nous delete
}



CGIReadHandler::~CGIReadHandler() { close(_fd); }
