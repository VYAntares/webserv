#include "../../includes/cgi/CGIProcess.hpp"

#include <unistd.h>
#include <cstdlib>
#include <stdexcept>

CGIProcess::CGIProcess(const HttpRequest& req, const Location* loc,
						std::string& path, std::string& interpreter,
						const std::string& peerAddr) 
						: _pid(-1), _write_fd(-1), _read_fd(-1) {
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
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);
		
		char* av[] = { (char*)interpreter.c_str(), (char*)path.c_str(), NULL };
		// path = "/var/www/site/scripts/hello.py"
		std::string dir = path.substr(0, path.rfind('/'));
		// dir = "/var/www/site/scripts"
		chdir(dir.c_str());

		execve(interpreter.c_str(), av, NULL);
		exit(1);
	// bloc parent
	} else {
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);
		_write_fd = pipe_stdin[1];				// parent ecrit ici et envoie a stdin[0] (enfant va lire)
		_read_fd = pipe_stdout[0];				// parent lit ici et recoit depuis stdout[1] (enfant va ecrire)
	}
}

int		CGIProcess::getWriteFd()	const { return _write_fd; }
int		CGIProcess::getReadFd()		const { return _read_fd; }
pid_t	CGIProcess::getPid()		const { return _pid; }

CGIProcess::~CGIProcess() {}

