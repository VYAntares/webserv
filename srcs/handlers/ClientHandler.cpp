#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/handlers/CGIReadHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/http/Router.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>

// ─── Cycle de vie ─────────────────────────────────────────────────────────────

ClientHandler::ClientHandler(int clientFd, const Server& server,
                             const struct sockaddr_in& peerAddr)
											: _fd(clientFd), _sent(0), _server(server),
											_parser(server), _rh(NULL), _keepAlive(false), 
											_lastActivity(time(NULL)), _cgiRead(NULL) {
	_peerAddr = _buildPeerStr(peerAddr);
	EventLoop::instance()->register_handler(this, READ_EVENT);
	std::cout << "[client " << _peerAddr << " fd=" << _fd << "] connected\n";
}



// Le destructeur ne delete pas _rh via _reset() pour éviter de rappeler
// EventLoop depuis un contexte de destruction. On delete directement.
// Si un CGI est encore en cours, on prévient son CGIReadHandler que nous
// n'existons plus : sans ça, il appellerait onCgiDone() sur un objet détruit.
ClientHandler::~ClientHandler() {
	if (_cgiRead)
		_cgiRead->detachSink();
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

	_rh = Router::route(req, _server, _peerAddr, this);
	if (!_rh) {
		// CGI -> async : la réponse arrivera via onCgiDone()
		// La réponse CGI est construite sans connaître notre keep-alive,
		// elle annonce toujours "Connection: close" → on ferme après envoi.
		_keepAlive = false;
		// Plus rien à lire tant que le CGI tourne : on ne garde que la
		// détection de déconnexion (EPOLLHUP/EPOLLERR restent signalés).
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
	HttpRequest req = _parser.getReq();		// contient req.error

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
	_cgiRead  = NULL; // le CGIReadHandler se détruit juste après, ne plus y toucher
	_sent     = 0;
	EventLoop::instance()->modify_handler(this, WRITE_EVENT);
}



// ─── handle_input ─────────────────────────────────────────────────────────────
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



// ─── handle_output ────────────────────────────────────────────────────────────
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
			// Une requête pipelinée peut déjà être entière dans le buffer du
			// parseur : aucun octet n'arrivera pour re-déclencher handle_input,
			// il faut donc re-parser le reliquat maintenant.
			std::string empty;
			_parser.runParsing(empty);
			if (_parser.getState() == HttpParser::COMPLETE)
				_handleComplete();
			else if (_parser.getState() == HttpParser::ERROR)
				_handleError();
			return 0;
		}
		return -1; // Connection: close → EventLoop détruira ce handler
	}
	return 0;
}



// ─── Helpers ─────────────────────────────────────────────────────────────────

// s_addr est un uint32_t déjà en network byte order (big-endian), donc son
// octet mémoire 0 est le 1er chiffre de l'IP ("a" dans a.b.c.d), l'octet 1
// le 2e, etc. On le relit comme 4 octets bruts pour éviter inet_ntoa (interdit).
std::string ClientHandler::_buildPeerStr(const struct sockaddr_in& addr) const {
	const unsigned char* ip = reinterpret_cast<const unsigned char*>(&addr.sin_addr.s_addr);
	std::ostringstream oss;
	oss << static_cast<int>(ip[0]) << "." << static_cast<int>(ip[1]) << "."
		<< static_cast<int>(ip[2]) << "." << static_cast<int>(ip[3])
		<< ":" << ntohs(addr.sin_port);
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



int		ClientHandler::getFd()			 const { return _fd; }
time_t	ClientHandler::getLastActivity() const { return _lastActivity; }
