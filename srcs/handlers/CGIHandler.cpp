#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/cgi/CGIProcess.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/handlers/CGIWriteHandler.hpp"
#include "../../includes/handlers/CGIReadHandler.hpp"

CGIHandler::CGIHandler(const HttpRequest& req, std::string& path, std::string& interpreter,
						const std::string& peerAddr, const Location* loc, IResponseSink* sink) {
	_process = new CGIProcess(req, path, interpreter, peerAddr);

	CGIWriteHandler* wr = new CGIWriteHandler(_process->getWriteFd(), req.body);
	EventLoop::instance()->register_handler(wr, WRITE_EVENT);

	CGIReadHandler* rd = new CGIReadHandler(_process->getReadFd(), _process->getPid(), loc, sink);
	EventLoop::instance()->register_handler(rd, READ_EVENT);
	if (sink)
		sink->onCgiStart(rd);
}

CGIHandler::~CGIHandler() {
	delete _process;
}

