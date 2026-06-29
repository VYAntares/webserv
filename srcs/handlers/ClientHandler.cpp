#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/http/Router.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>

// ─── Helpers ─────────────────────────────────────────────────────────────────

std::string ClientHandler::_buildPeerStr(const struct sockaddr_in& addr) const {
	std::ostringstream oss;
	oss << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
	return oss.str();
}

// Appelé après chaque requête complète (keep-alive) pour préparer le prochain cycle.
// _response est vidée explicitement pour libérer la mémoire allouée par la string.
void ClientHandler::_reset() {
	delete _rh;
	_rh = NULL;
	_response.clear();
	_sent = 0;
	_parser.reset();
}

int ClientHandler::getFd() const { return _fd; }

// ─── Cycle de vie ─────────────────────────────────────────────────────────────

ClientHandler::ClientHandler(int clientFd, const Server& server,
                             const struct sockaddr_in& peerAddr)
											: _fd(clientFd), _sent(0), _server(server),
											_parser(server.max_body_client), _rh(NULL),
											_keepAlive(false) {
	_peerAddr = _buildPeerStr(peerAddr);
	EventLoop::instance()->register_handler(this, READ_EVENT);
	std::cout << "[client " << _peerAddr << " fd=" << _fd << "] connected\n";
}

// Le destructeur ne delete pas _rh via _reset() pour éviter de rappeler
// EventLoop depuis un contexte de destruction. On delete directement.
ClientHandler::~ClientHandler() {
	delete _rh;
	close(_fd);
	std::cout << "[client " << _peerAddr << " fd=" << _fd << "] disconnected\n";
}

void	ClientHandler::_handleComplete() {
	HttpRequest req = _parser.getReq();

	// Décider keep-alive avant de construire la réponse :
	// HTTP/1.1 est keep-alive par défaut, HTTP/1.0 est close par défaut.
	std::map<std::string, std::string>::const_iterator it =
	    req.headers.find("Connection");
	if (it != req.headers.end())
		_keepAlive = (it->second == "keep-alive");
	else
		_keepAlive = (req.version == "HTTP/1.1");

	_rh = Router::route(req, _server, _peerAddr);
	_response = _rh->buildResponse();

	// debugger
	std::cout << "[client " << _peerAddr << "] "
	          << req.method << " " << req.uri << "\n";

	EventLoop::instance()->modify_handler(this, WRITE_EVENT);
}

void	ClientHandler::_handleError() {
	HttpRequest req = _parser.getReq();		// contient req.error

	_keepAlive = false;
	_rh = Router::route(req, _server, _peerAddr);
	_response = _rh->buildResponse();

	EventLoop::instance()->modify_handler(this, WRITE_EVENT);
}

// ─── handle_input ─────────────────────────────────────────────────────────────
int ClientHandler::handle_input() {
	char buf[4096];
	ssize_t n = recv(_fd, buf, sizeof(buf), 0);

	if (n <= 0)
		return -1;

	std::string data(buf, n);
	_parser.runParsing(data, static_cast<size_t>(n));

	if (_parser.getState() == HttpParser::COMPLETE)
		_handleComplete();

	else if (_parser.getState() == HttpParser::ERROR)
		_handleError();

	return 0;
}

// ─── handle_output ────────────────────────────────────────────────────────────
int ClientHandler::handle_output() {
	const char*  data = _response.c_str() + _sent;
	size_t       left = _response.size() - _sent;

	ssize_t n = send(_fd, data, left, MSG_NOSIGNAL);

	if (n <= 0)
		return -1;

	_sent += static_cast<size_t>(n);

	if (_sent >= _response.size()) {
		if (_keepAlive) {
			_reset();
			EventLoop::instance()->modify_handler(this, READ_EVENT);
			return 0;
		}
		return -1; // Connection: close → EventLoop détruira ce handler
	}

	return 0;
}
