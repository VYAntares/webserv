#include "../../includes/handlers/CGIReadHandler.hpp"
#include <unistd.h>
#include <sstream>
#include <iostream>

CGIReadHandler::CGIReadHandler(int fd, pid_t pid, IResponseSink* sink) 
							: _fd(fd), _pid(pid), _sink(sink) {
}


int CGIReadHandler::handle_input() {
	// read()
	// waitpid
	// raw = parseCgiOutput
    std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n";
    oss	<< "Content-Type: text/html\r\n";
	oss << "Content-Length: 0\r\n"
		<< "Connection: close\r\n"
		<< "\r\n";

	std::string r = oss.str();
    std::cout << "REEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEADING" << std::endl;
	_sink->onCgiDone(r);
	close(_fd);
	return 0;
}
