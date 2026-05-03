#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/http/HttpParser.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/Router.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <sstream>

static std::string make_response() {
	std::string body(1024, 'B'); // 10MB de 'A'
	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/plain\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return oss.str();
}

static const std::string response = make_response();

ClientHandler::ClientHandler(int clientFd, const Server& server)
							 : _fd(clientFd), _sent(0), _server(server), 
							   _parser(server.max_body_client) {
	EventLoop::instance()->register_handler(this, READ_EVENT);
}

ClientHandler::~ClientHandler() { close(_fd); }

int ClientHandler::getFd() const { return _fd; }

int ClientHandler::handle_input() {
	char buf[4096];
	ssize_t n = recv(_fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0)
		return -1;
	buf[n] = '\0';

	std::string data(buf, n);
	_parser.runParsing(data, n);

	if (_parser.getState() == HttpParser::COMPLETE) {
		HttpRequest _request = _parser.getReq();
    	_request.error = Router::routeChecker(_request, _server);
		if (_request.error != 200)
			return -1;
		std::cout << _request.error << std::endl;
    	// _parser.reset();
		EventLoop::instance()->modify_handler(this, WRITE_EVENT);
		return 0;
	}
	else if (_parser.getState() == HttpParser::ERROR)
		return -1;
	
	return 0;
}

int ClientHandler::handle_output() {
	ssize_t n = send(_fd, response.c_str() + _sent, response.size() - _sent, 0);
	if (n <= 0)
		return -1;
	_sent += n;
	if (_sent >= response.size()) {
		_sent = 0;
		EventLoop::instance()->modify_handler(this, READ_EVENT);
		return 0;
	}
	return 0;
}
