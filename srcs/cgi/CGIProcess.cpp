#include "../../includes/cgi/CGIProcess.hpp"

#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <vector>

// ┌───────────────────┬─────────────────────────────────────────────────────────────────────────┐
// │     Variable      │                          Source dans le code                            │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ REQUEST_METHOD    │ _req.method                                                             │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ QUERY_STRING      │ partie après ? dans _req.uri                                            │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ SCRIPT_NAME       │ partie avant ? dans _req.uri                                            │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ SCRIPT_FILENAME   │ le path passé à CGIFork                                                 │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ SERVER_PROTOCOL   │ _req.version                                                            │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ GATEWAY_INTERFACE │ constante "CGI/1.1"                                                     │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ SERVER_SOFTWARE   │ constante "webserv/1.0"                                                 │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ CONTENT_TYPE      │ _req.headers["Content-Type"]                                            │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ CONTENT_LENGTH    │ _req.headers["Content-Length"] ou _req.body.size()                      │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ REMOTE_ADDR       │ _peerAddr                                                               │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ SERVER_NAME       │ partie host du header "Host"                                            │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ SERVER_PORT       │ partie port du header "Host"                                            │
// ├───────────────────┼─────────────────────────────────────────────────────────────────────────┤
// │ HTTP_*            │ tous les autres headers, transformés : tirets → underscores, MAJUSCULES │
// └───────────────────┴─────────────────────────────────────────────────────────────────────────┘

CGIProcess::CGIProcess(const HttpRequest& req, const Location* loc,
						std::string& path, std::string& interpreter,
						const std::string& peerAddr) 
						: _req(req), _loc(loc), _peerAddr(peerAddr),
						  _pid(-1), _write_fd(-1), _read_fd(-1) {
	// Un pipe() crée 2 fd : [0] pour lire, [1] pour écrire. On en crée deux :
	// pipe_stdin[0]  → enfant lit  (son STDIN)
	// pipe_stdin[1]  → parent écrit le body dedans
	// pipe_stdout[0] → parent lit la réponse du CGI
	// pipe_stdout[1] → enfant écrit (son STDOUT)
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

void	CGIProcess::CGIFork(int* pipe_stdout, int* pipe_stdin, 
							std::string& interpreter, std::string& path) {
	_pid = fork();
	// erreur, fermer tout les pipes.
	if (_pid == -1) {
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		throw std::runtime_error("fork() failed");
	}
	// bloc enfant
	if (_pid == 0) {
		dup2(pipe_stdin[0], STDIN_FILENO);		// connecter le stdin au pipe (enfant lit)
		dup2(pipe_stdout[1], STDOUT_FILENO);	// connecter le stdout au pipe (enfant ecrit)
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);

		char* av[] = { (char*)interpreter.c_str(), (char*)path.c_str(), NULL };
		// path = "/var/www/site/scripts/hello.py"
		std::string dir = path.substr(0, path.rfind('/'));
		// dir = "/var/www/site/scripts"
		chdir(dir.c_str());

		char** envp = buildEnvp(path);
		execve(interpreter.c_str(), av, envp);
		exit(1);
	// bloc parent
	} else {
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);
		_write_fd = pipe_stdin[1];				// parent ecrit ici et envoie a stdin[0] (enfant va lire)
		_read_fd = pipe_stdout[0];				// parent lit ici et recoit depuis stdout[1] (enfant va ecrire)
	}
}

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

void CGIProcess::addRemoteAddr(std::vector<std::string>* envp) {
	size_t sep = _peerAddr.find(':');
	if (sep != std::string::npos) {
		envp->push_back("REMOTE_ADDR=" + _peerAddr.substr(0, sep));
		envp->push_back("REMOTE_PORT=" + _peerAddr.substr(sep + 1));
	} else {
    	envp->push_back("REMOTE_ADDR=" + _peerAddr);
	}
}

void CGIProcess::addUri(std::vector<std::string>* envp) {
	size_t pos = _req.uri.find('?');
	if (pos != std::string::npos) {
		envp->push_back("SCRIPT_NAME=" + _req.uri.substr(0, pos));
		envp->push_back("QUERY_STRING=" + _req.uri.substr(pos + 1));
	} else {
		envp->push_back("SCRIPT_NAME=" + _req.uri);
		envp->push_back("QUERY_STRING=");
	}
}

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

