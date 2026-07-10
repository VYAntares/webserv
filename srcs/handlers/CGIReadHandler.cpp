#include "../../includes/handlers/CGIReadHandler.hpp"
#include "../../includes/http/ErrorHandler.hpp"
#include "../../includes/http/StaticHandler.hpp"
#include "../../includes/utils/utils.hpp"
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>

CGIReadHandler::CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink)
							: _fd(fd), _pid(pid), _loc(loc), _sink(sink),
							  _reaped(false), _responded(false), _lastActivity(time(NULL)) {
}

// Récupère le statut de l'enfant sans jamais bloquer la boucle : si le CGI a
// fermé son stdout mais tourne encore (ex: boucle infinie après le print),
// l'ancien waitpid bloquant gelait TOUT le serveur. On le tue proprement et
// on sert quand même la sortie déjà reçue.
void CGIReadHandler::reapChild(int& status) {
	pid_t r = waitpid(_pid, &status, WNOHANG);
	if (r == 0) {
		kill(_pid, SIGKILL);
		waitpid(_pid, &status, 0);
		status = 0; // la sortie reçue est complète : on la considère valide
	}
	_reaped = true;
}

// La sortie d'un CGI n'est pas une réponse HTTP : c'est un bloc de headers
// CGI (Status:, Content-Type:, ...) suivi d'une ligne vide puis du body
// (RFC 3875). Avant, la sortie brute était collée telle quelle dans le body
// de la réponse → les headers du script s'affichaient dans la page.
std::string CGIReadHandler::buildCgiResponse() {
	int			code = 200;
	std::string	type;
	std::string	body;

	size_t sep	= _out.find("\r\n\r\n");
	size_t skip	= 4;
	if (sep == std::string::npos) { // certains scripts n'émettent que du \n
		sep  = _out.find("\n\n");
		skip = 2;
	}
	if (sep == std::string::npos) // pas de bloc header : tout est body
		body = _out;
	else {
		std::string headers = _out.substr(0, sep);
		body = _out.substr(sep + skip);

		std::istringstream iss(headers);
		std::string line;
		while (std::getline(iss, line)) {
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			if (line.compare(0, 7, "Status:") == 0)
				code = std::atoi(line.c_str() + 7);
			else if (line.compare(0, 13, "Content-Type:") == 0) {
				size_t v = 13;
				while (v < line.size() && line[v] == ' ')
					v++;
				type = line.substr(v);
			}
		}
	}
	if (code <= 0)
		code = 200;

	StaticHandler s(*_loc, code, body);
	if (!type.empty())
		s.setType(type);
	return s.buildResponse();
}

int CGIReadHandler::handle_input() {
	_lastActivity = time(NULL);

	char buf[4096];
	ssize_t n = read(_fd, buf, sizeof(buf));

	if (n > 0) {
		_out.append(buf, n);
		return 0;								// pas fini : epoll nous rappellera
	}
	if (n < 0) {								// erreur réelle (epoll a dit "lisible")
		if (_sink) {
			ErrorHandler err(*_loc, 502);
			_sink->onCgiDone(err.buildResponse());	// ne pas laisser le client sans réponse
			_responded = true;
		}
		return -1;								// l'enfant sera tué/reapé par le destructeur
	}

	// n == 0 : EOF — le CGI a fermé son stdout, _out est complet
	int status = 0;
	reapChild(status);

	// client déjà déconnecté : _loc pointe dans son Server, ne plus y toucher
	if (!_sink)
		return -1;

	std::string raw;
	if (WIFSIGNALED(status)) {
		ErrorHandler err(*_loc, 502);
		raw = err.buildResponse();
	}
	else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		ErrorHandler err(*_loc, 500, _out);
		raw = err.buildResponse();
	}
	else
		raw = buildCgiResponse();

	if (_sink) {								// NULL si le client a déjà disparu
		_sink->onCgiDone(raw);
		_responded = true;
	}
	return -1;									// EventLoop nous retire et nous delete
}

// Trois responsabilités à la destruction (EOF normal, timeout, arrêt serveur) :
// 1. fermer notre bout du pipe
// 2. tuer + reaper l'enfant s'il ne l'a pas déjà été (pas de zombie)
// 3. si aucun client n'a encore reçu de réponse (timeout CGI), envoyer 504 —
//    "une requête ne doit jamais rester en attente indéfiniment" (sujet)
CGIReadHandler::~CGIReadHandler() {
	close(_fd);
	if (!_reaped) {
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, 0);
	}
	if (_sink && !_responded) {
		ErrorHandler err(*_loc, 504);
		_sink->onCgiDone(err.buildResponse());
	}
}
