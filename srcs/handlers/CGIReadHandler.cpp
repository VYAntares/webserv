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
	int status;
	std::string raw;
	ARequestHandler *a = NULL;

	waitpid(_pid, &status, 0);
	int sig = 0;
	if (WIFEXITED(status)) {
		sig = WEXITSTATUS(status);
		if (sig != 0) // code 500
			a = new ErrorHandler(*_loc, 500);
	} else if (WIFSIGNALED(status))
		a = new ErrorHandler(*_loc, 500);
		
	if (!a) {
		std::string	out;
		char buf[4096];
		while (read(_fd, buf, sizeof(buf)) > 0)
		out.append(buf);
		a = new StaticHandler(*_loc, 200, out);
	}
	close(_fd);
	raw = a->buildResponse();
	_sink->onCgiDone(raw);
	return 0;
}
