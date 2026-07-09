#include "../../includes/cgi/CGIProcess.hpp"

#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <fcntl.h>
#include <cerrno>
#include <signal.h>
#include <sys/wait.h>

// Un pipe() crée 2 fd : [0] pour lire, [1] pour écrire. On en crée deux :
// pipe_stdin[0]  → enfant lit  (son STDIN)
// pipe_stdin[1]  → parent écrit le body dedans
// pipe_stdout[0] → parent lit la réponse du CGI
// pipe_stdout[1] → enfant écrit (son STDOUT)
CGIProcess::CGIProcess(const HttpRequest& req, std::string& path, 
						std::string& interpreter, const std::string& peerAddr) 
						: _req(req), _peerAddr(peerAddr),
						  _pid(-1), _write_fd(-1), _read_fd(-1) {
	int pipe_stdin[2];
	int pipe_stdout[2];
	if (pipe(pipe_stdin) == -1)
		throw std::runtime_error("pipe() stdin failed");
	if (pipe(pipe_stdout) == -1) {
		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		throw std::runtime_error("pipe() stdout failed");
	}
	CGIFork(pipe_stdout, pipe_stdin, interpreter, path);
}

// fork() duplique le processus courant : les deux copies partagent à cet
// instant les 4 fd de pipe. Chacune ne garde que ce qui la concerne et ferme
// le reste — règle d'or des pipes : tant qu'un fd d'écriture traîne ouvert
// quelque part (même par erreur, dans le mauvais processus), le lecteur en
// face ne voit jamais l'EOF et peut rester bloqué indéfiniment.
//
//   pipe_stdin[0]  --lecture-->  enfant  (dup2 → STDIN_FILENO)
//   pipe_stdin[1]  --écriture--> parent  (_write_fd)
//   pipe_stdout[0] --lecture-->  parent  (_read_fd)
//   pipe_stdout[1] --écriture--> enfant  (dup2 → STDOUT_FILENO)
void	CGIProcess::CGIFork(int* pipe_stdout, int* pipe_stdin,
							std::string& interpreter, std::string& path) {
	_pid = fork();
	// erreur, fermer tout les pipes.
	if (_pid == -1) {
		close(pipe_stdout[0]); close(pipe_stdout[1]);
		close(pipe_stdin[0]); close(pipe_stdin[1]);
		throw std::runtime_error("fork() failed");
	}

	// bloc enfant
	if (_pid == 0) {
		dup2(pipe_stdin[0], STDIN_FILENO);		// connecter le stdin au pipe (enfant lit)
		dup2(pipe_stdout[1], STDOUT_FILENO);	// connecter le stdout au pipe (enfant ecrit)
		close(pipe_stdin[0]); close(pipe_stdin[1]);
		close(pipe_stdout[0]); close(pipe_stdout[1]);

		// On se place dans le dossier du script (accès aux fichiers relatifs,
		// exigé par le sujet). L'argument passé à l'interpréteur doit alors
		// être le SEUL nom de fichier : passer "www/cgi-bin/hello.py" après
		// un chdir dans "www/cgi-bin" ferait chercher un chemin dédoublé.
		size_t slash = path.rfind('/');
		std::string dir  = (slash == std::string::npos) ? "." : path.substr(0, slash);
		std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
		if (chdir(dir.c_str()) == -1)
			exit(1);

		char** envp = buildEnvp(path);
		char* av[] = { (char*)interpreter.c_str(), (char*)file.c_str(), NULL };
		execve(interpreter.c_str(), av, envp);
		exit(1);

	// bloc parent
	} else {
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);
		_write_fd = pipe_stdin[1];				// parent ecrit ici et envoie a stdin[0] (enfant va lire)
		_read_fd = pipe_stdout[0];				// parent lit ici et recoit depuis stdout[1] (enfant va ecrire)

		// Non-bloquant seulement côté parent : chaque extrémité d'un pipe est
		// une open file description à part, donc ça ne touche pas le stdin/
		// stdout (bloquants) de l'enfant. Indispensable avec epoll : sinon un
		// read()/write() peut bloquer tout le event loop si le pipe est plein
		// ou vide au moment de l'appel.
		if (fcntl(_write_fd, F_SETFL, fcntl(_write_fd, F_GETFL) | O_NONBLOCK) == -1
			|| fcntl(_read_fd, F_SETFL, fcntl(_read_fd, F_GETFL) | O_NONBLOCK) == -1) {
			// nettoyer avant de throw : sinon fds et process enfant fuient
			close(_write_fd);
			close(_read_fd);
			kill(_pid, SIGKILL);
			waitpid(_pid, NULL, 0);
			throw std::runtime_error("fcntl() failed: " + std::string(strerror(errno)));
		}

		// FD_CLOEXEC : si un AUTRE CGI est forké pendant que celui-ci tourne,
		// l'enfant du second hériterait de nos deux fds de pipe — le premier
		// CGI ne verrait alors jamais l'EOF sur son stdin (fd d'écriture
		// encore ouvert ailleurs) et resterait bloqué.
		fcntl(_write_fd, F_SETFD, FD_CLOEXEC);
		fcntl(_read_fd, F_SETFD, FD_CLOEXEC);
	}
}

// execve() attend un char** terminé par NULL, mais on ne connaît pas à
// l'avance le nombre de variables d'environnement. On les accumule d'abord
// dans un vector<string> (taille dynamique), puis on convertit une seule fois
// à la fin via convertToCharStarStarBabyyy().
char** CGIProcess::buildEnvp(std::string& path) {
	std::vector<std::string> envp;
	
	std::ostringstream oss;
	oss << _req.body.size();
	envp.push_back("CONTENT_LENGTH=" + oss.str());
	
	envp.push_back("REQUEST_METHOD=" + _req.method);
	envp.push_back("SCRIPT_FILENAME=" + path);
	envp.push_back("SERVER_PROTOCOL=" + _req.version);
	envp.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envp.push_back("SERVER_SOFTWARE=webserv/1.0");

	addUri(&envp);
	addHost(&envp);
	addHeaders(&envp);
	addRemoteAddr(&envp);

	return convertToCharStarStarBabyyy(&envp);
}

// Convertit le vector<string> en char** NULL-terminated attendu par execve().
// strdup() alloue chaque chaîne séparément ; ces allocations ne sont jamais
// libérées explicitement, mais ça n'a pas d'importance ici : soit execve()
// réussit et remplace tout l'espace mémoire du processus (le tas actuel
// disparaît avec), soit elle échoue et exit(1) suit juste après.
char** CGIProcess::convertToCharStarStarBabyyy(std::vector<std::string>* envp) {
	char** env;

	env = (char**)malloc((envp->size() + 1) * sizeof(char*));
	if (!env)
		throw std::runtime_error("malloc() on env failed");

	for (size_t i = 0; i < envp->size(); i++) {
		std::string& s = (*envp)[i];
		env[i] = strdup(s.c_str());
	}
	env[envp->size()] = NULL;

	return env;
}

// REMOTE_ADDR/REMOTE_PORT : adresse du client qui a fait la requête HTTP
// (rien à voir avec les pipes CGI). _peerAddr arrive déjà formaté "ip:port" ;
// si jamais le ':' est absent, on retombe sur l'adresse seule sans port.
void CGIProcess::addRemoteAddr(std::vector<std::string>* envp) {
	size_t sep = _peerAddr.find(':');
	if (sep != std::string::npos) {
		envp->push_back("REMOTE_ADDR=" + _peerAddr.substr(0, sep));
		envp->push_back("REMOTE_PORT=" + _peerAddr.substr(sep + 1));
	} else {
    	envp->push_back("REMOTE_ADDR=" + _peerAddr);
	}
}

// SCRIPT_NAME = chemin avant le '?', QUERY_STRING = ce qui suit.
// Exemple : "/cgi-bin/multiply.py?a=4&b=7"
//   → SCRIPT_NAME=/cgi-bin/multiply.py
//   → QUERY_STRING=a=4&b=7
// C'est cette dernière que le script relit pour récupérer les paramètres
// d'un GET (ex. os.environ["QUERY_STRING"] côté Python).
void CGIProcess::addUri(std::vector<std::string>* envp) {
	size_t pos = _req.uri.find('?');
	std::string scriptPath = (pos != std::string::npos) ? _req.uri.substr(0, pos) : _req.uri;
	envp->push_back("SCRIPT_NAME=" + scriptPath);
	envp->push_back("QUERY_STRING=" + (pos != std::string::npos ? _req.uri.substr(pos + 1) : ""));
	// PATH_INFO et REQUEST_URI obligatoires pour cgi_test (42) : il vérifie
	// que PATH_INFO existe ET correspond à REQUEST_URI, sinon 500
	// ("PATH_INFO not found" / "PATH_INFO incorrect").
	envp->push_back("PATH_INFO=" + scriptPath);
	envp->push_back("REQUEST_URI=" + _req.uri);
}

// SERVER_NAME/SERVER_PORT extraits du header "Host: nom[:port]".
// Port 80 par défaut si absent du header — un choix arbitraire : le vrai port
// d'écoute configuré pour ce serveur n'est pas transmis jusqu'à CGIProcess.
void CGIProcess::addHost(std::vector<std::string>* envp) {
	std::map<std::string, std::string>::const_iterator it;
	
	it = _req.headers.find("Host");
	std::string host = (it != _req.headers.end() ? it->second : "");
	std::string serverName = host;
	std::string serverPort = "80";
	size_t hostSep = host.find(':');
	if (hostSep != std::string::npos) {
		envp->push_back("SERVER_NAME=" + host.substr(0, hostSep));
		envp->push_back("SERVER_PORT=" + host.substr(hostSep + 1));
	} else {
		envp->push_back("SERVER_NAME=" + host);
		envp->push_back("SERVER_PORT=" + serverPort);
	}
}

// Convertit chaque header restant en variable HTTP_<NOM_EN_MAJUSCULES>,
// tirets remplacés par underscores (convention CGI/1.1, RFC 3875) :
// ex. "User-Agent: curl/8.0"  →  HTTP_USER_AGENT=curl/8.0
// Content-Type et Content-Length sont exclus de cette boucle : ils ont déjà
// leurs propres variables dédiées (CONTENT_TYPE, CONTENT_LENGTH) plus haut,
// pas de préfixe HTTP_ pour eux.
void CGIProcess::addHeaders(std::vector<std::string>* envp) {
	std::map<std::string, std::string>::const_iterator it;
	
	it = _req.headers.find("Content-Type");
	std::string contentType = (it != _req.headers.end() ? it->second : "");
	envp->push_back("CONTENT_TYPE=" + contentType);
	
	for (it = _req.headers.begin(); it != _req.headers.end(); ++it) {
		const std::string& key = it->first;
		if (key == "Content-Type" || key == "Content-Length")
			continue;

		std::string name = "HTTP_";
		for (size_t i = 0; i < key.size(); i++) {
			if (key[i] == '-')
				name += '_';
			else
				name += std::toupper(static_cast<unsigned char>(key[i]));
		}
		envp->push_back(name + "=" + it->second);
	}
}

int		CGIProcess::getWriteFd()	const { return _write_fd; }
int		CGIProcess::getReadFd()		const { return _read_fd; }
pid_t	CGIProcess::getPid()		const { return _pid; }

CGIProcess::~CGIProcess() {}

