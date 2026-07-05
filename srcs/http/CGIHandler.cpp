#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/cgi/CGIProcess.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/handlers/CGIWriteHandler.hpp"
#include "../../includes/handlers/CGIReadHandler.hpp"

CGIHandler::CGIHandler(const HttpRequest& req, std::string& path,
						std::string& interpreter, const std::string& peerAddr,
						IResponseSink* sink) {
	// Processus CGI (fork + exec + pipes)
	_process = new CGIProcess(req, path, interpreter, peerAddr);
	
	// brancher les pipes
	EventLoop::instance()->register_handler(
			new CGIWriteHandler(_process->getWriteFd(), req.body), WRITE_EVENT);
	EventLoop::instance()->register_handler(
			new CGIReadHandler(_process->getReadFd(), _process->getPid(), sink), READ_EVENT);
}

CGIHandler::~CGIHandler() {
	delete _process;
}

