#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/handlers/CGIReadHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/http/Router.hpp"
#include "../../includes/http/ErrorHandler.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>

ClientHandler::ClientHandler(int clientFd, const Server& server,
                             const struct sockaddr_in& peerAddr)
											: _fd(clientFd), _sent(0), _server(server),
											_parser(server), _rh(NULL), _keepAlive(false), 
											_lastActivity(time(NULL)), _cgiRead(NULL) {
	_peerAddr = _buildPeerStr(peerAddr);
	EventLoop::instance()->register_handler(this, READ_EVENT);
	std::cout << "[client " << _peerAddr << " fd=" << _fd << "] connected\n";
}

ClientHandler::~ClientHandler() {
	if (_cgiRead)
		_cgiRead->detachSink();
	delete _rh;
	close(_fd);
	std::cout << "[client " << _peerAddr << " fd=" << _fd << "] disconnected\n";
}

void	ClientHandler::_handleComplete() {
	HttpRequest req = _parser.getReq();

	std::map<std::string, std::string>::const_iterator it =
	    req.headers.find("Connection");
	if (it != req.headers.end())
		_keepAlive = (it->second == "keep-alive");
	else
		_keepAlive = (req.version == "HTTP/1.1");

	_rh = Router::route(req, _server, _peerAddr, this);
	if (!_rh) {
		_keepAlive = false;
		EventLoop::instance()->modify_handler(this, static_cast<EventType>(0));
		return;
	}
	if (req.method == "HEAD") {
		_rh->stripBody();
		_keepAlive = false;
	}
	_rh->setKeepAlive(_keepAlive);
	_response = _rh->buildResponse();

	std::cout << "[client " << _peerAddr << "] "
	          << req.method << " " << req.uri << "\n";

	EventLoop::instance()->modify_handler(this, WRITE_EVENT);
}

void	ClientHandler::_handleError() {
	HttpRequest req = _parser.getReq();

	std::cout << "[client " << _peerAddr << "] "
			  << req.method << " " << req.uri
			  << " (error " << req.error << ")\n";

	_keepAlive = false;
	_rh = Router::route(req, _server, _peerAddr, this);
	_response = _rh->buildResponse();

	EventLoop::instance()->modify_handler(this, WRITE_EVENT);
}

void	ClientHandler::onCgiStart(CGIReadHandler* rd) {
	_cgiRead = rd;
}

void	ClientHandler::onCgiDone(const std::string& rawHttpResp) {
	_response = rawHttpResp;
	_cgiRead  = NULL;
	_sent     = 0;
	EventLoop::instance()->modify_handler(this, WRITE_EVENT);
}

int ClientHandler::handle_input() {
	_lastActivity = time(NULL);

	char buf[4096];
	ssize_t n = recv(_fd, buf, sizeof(buf), 0);
	if (n <= 0)
		return -1;
	std::string data(buf, n);

	_parser.runParsing(data);
	if (_parser.getState() == HttpParser::COMPLETE)
		_handleComplete();
	else if (_parser.getState() == HttpParser::ERROR)
		_handleError();

	return 0;
}

int ClientHandler::handle_output() {
	_lastActivity = time(NULL);

	const char*  data = _response.c_str() + _sent;
	size_t       left = _response.size()  - _sent;

	ssize_t n = send(_fd, data, left, MSG_NOSIGNAL);
	if (n <= 0)
		return -1;

	_sent += static_cast<size_t>(n);
	if (_sent >= _response.size()) {
		if (_keepAlive) {
			_reset();
			EventLoop::instance()->modify_handler(this, READ_EVENT);
			std::string empty;
			_parser.runParsing(empty);
			if (_parser.getState() == HttpParser::COMPLETE)
				_handleComplete();
			else if (_parser.getState() == HttpParser::ERROR)
				_handleError();
			return 0;
		}
		return -1;
	}
	return 0;
}

std::string ClientHandler::_buildPeerStr(const struct sockaddr_in& addr) const {
	const unsigned char* ip = reinterpret_cast<const unsigned char*>(&addr.sin_addr.s_addr);
	std::ostringstream oss;
	oss << static_cast<int>(ip[0]) << "." << static_cast<int>(ip[1]) << "."
		<< static_cast<int>(ip[2]) << "." << static_cast<int>(ip[3])
		<< ":" << ntohs(addr.sin_port);
	return oss.str();
}

void ClientHandler::_reset() {
	delete _rh;
	_rh = NULL;
	_response.clear();
	_sent = 0;
	_parser.reset();
}

int	ClientHandler::handle_timeout() {
	if (!_cgiRead)
		return 0;

	ErrorHandler err(_server, 504, "");
	_response = err.buildResponse();
	_sent = 0;
	_keepAlive = false;
	EventLoop::instance()->modify_handler(this, WRITE_EVENT);
	return 1;
}

int		ClientHandler::getFd()			 const { return _fd; }
time_t	ClientHandler::getLastActivity() const { return _lastActivity; }
