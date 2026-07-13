#include "../../includes/handlers/CGIReadHandler.hpp"
#include "../../includes/http/ErrorHandler.hpp"
#include "../../includes/http/StaticHandler.hpp"
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <sys/wait.h>

CGIReadHandler::CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink) 
							: _fd(fd), _pid(pid), _loc(loc), _sink(sink),
							  _lastActivity(time(NULL)), _reaped(false), _responded(false) {
}



// Récupère le statut de l'enfant sans jamais bloquer la boucle : si le CGI a
// fermé son stdout mais tourne encore (ex: boucle infinie après le print),
// l'ancien waitpid bloquant gelait TOUT le serveur. On le tue proprement et
// on sert quand même la sortie déjà reçue.
void CGIReadHandler::reapChild(int& status) {
	// WNOHANG dit au noyau : "regarde si le process _pid est déjà mort. 
	// S'il l'est, rends-moi son status. 
	// S'il ne l'est PAS, ne m'endors pas, rends-moi 0 tout de suite."
	pid_t r = waitpid(_pid, &status, WNOHANG);
	if (r == 0) {
		kill(_pid, SIGKILL);
		waitpid(_pid, &status, 0);
		status = 0;
	}
	_reaped = true;
}



// La sortie d'un CGI n'est pas une réponse HTTP : c'est un bloc de headers
// CGI (Status:, Content-Type:, ...) suivi d'une ligne vide puis du body
// (RFC 3875). Avant, la sortie brute était collée telle quelle dans le body
// de la réponse → les headers du script s'affichaient dans la page.
std::string CGIReadHandler::parseCgi() {
	int			code = 200;
	std::string	type;
	std::string	body;

	size_t sep = _out.find("\r\n\r\n");
	size_t s = 4;
	if (sep == std::string::npos) {
		sep = _out.find("\n\n");
		s   = 2;
	}
	if (sep == std::string::npos)
		body = _out;
	else {
		std::string headers = _out.substr(0, sep);
		body				= _out.substr(sep + s);

		std::istringstream	iss(headers);
		std::string			line;
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

	StaticHandler st(*_loc, code, body);
	if (!type.empty())
		st.setType(type);
	
	return st.buildResponse();	
}


int CGIReadHandler::handle_input() {
	_lastActivity = time(NULL);
	
	char buf[4096];
	ssize_t n = read(_fd, buf, sizeof(buf));

	if (n > 0) {
		_out.append(buf, n);
		// pas fini : epoll nous rappellera
		return 0;
	}
	if (n < 0) {
		if (_sink) {
			// erreur réelle (epoll a dit "lisible")
			ErrorHandler err(*_loc, 502);
			// ne pas laisser le client sans réponse
			_sink->onCgiDone(err.buildResponse());
			_responded = true;
		}
		return -1;
	}

	// n == 0 : EOF — le CGI a fermé son stdout, _out est complet
	std::cerr << "[DEBUG CGIReadHandler EOF] _out.size()=" << _out.size() << std::endl;
	int status = 0;
	reapChild(status);
	std::cerr << "[DEBUG CGIReadHandler status] WIFEXITED=" << WIFEXITED(status)
			  << " WEXITSTATUS=" << (WIFEXITED(status) ? WEXITSTATUS(status) : -1)
			  << " WIFSIGNALED=" << WIFSIGNALED(status) << std::endl;

	if (!_sink)
		return -1;
	
	std::string raw;
	// Retourne vrai si le processus a été tué par un signal
	// qu'il n'a pas géré lui-même — 
	// par exemple un segfault (SIGSEGV), un SIGKILL, un SIGTERM, etc. 
	// Dans ce cas il n'y a pas de "code de sortie" au sens normal, 
	// donc WEXITSTATUS n'a pas de sens ici.
	if (WIFSIGNALED(status)) {
		ErrorHandler err(*_loc, 502);
		raw = err.buildResponse();
	}
	// WIFEXITED(status) -> Retourne vrai si le processus enfant s'est terminé normalement, 
	// c'est-à-dire via un return dans main() ou un appel à exit().
	// WEXITSTATUS(status) -> À n'utiliser que si WIFEXITED est vrai. 
	// Extrait le code de sortie que le processus a passé à exit() 
	// (la valeur entre 0 et 255). C'est l'équivalent du $? dans un shell.
	else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		ErrorHandler err(*_loc, 500, _out);
		raw = err.buildResponse();
	}
	else 
		raw = parseCgi();

	if (_sink) {
		_sink->onCgiDone(raw);
		_responded = true;
	}

	return -1;			// EventLoop nous retire et nous delete
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
	// destructeur appelé sans passer par handle_input() (ex: timeout après
	// 30s d'inactivité dans EventLoop::checkTimeOut) -> client jamais servi
	if (_sink && !_responded) {
		ErrorHandler err(*_loc, 504);
		_sink->onCgiDone(err.buildResponse());
	}
}
