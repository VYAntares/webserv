#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/cgi/CGIProcess.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/handlers/CGIWriteHandler.hpp"
#include "../../includes/handlers/CGIReadHandler.hpp"
#include <sstream>

CGIHandler::CGIHandler(const HttpRequest& req, const Location* loc,
						std::string& path, std::string& interpreter,
						const std::string& peerAddr)
						: _req(req), _loc(loc),
						_path(path), _interpreter(interpreter),
						_peerAddr(peerAddr) {
	// Processus CGI (fork + exec + pipes)
	_process = new CGIProcess(_req, _loc, _path, _interpreter, _peerAddr);
	
	// brancher les pipes
	EventLoop::instance()->register_handler(
			new CGIWriteHandler(_process->getWriteFd()), WRITE_EVENT);
	EventLoop::instance()->register_handler(
			new CGIReadHandler(_process->getReadFd()), READ_EVENT);
}

CGIHandler::~CGIHandler() {
	delete _process;
}

// std::string CGIHandler::buildResponse() {
// }

