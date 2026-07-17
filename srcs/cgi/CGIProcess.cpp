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

void	CGIProcess::CGIFork(int* pipe_stdout, int* pipe_stdin,
							std::string& interpreter, std::string& path) {
	_pid = fork();
	if (_pid == -1) {
		close(pipe_stdout[0]); close(pipe_stdout[1]);
		close(pipe_stdin[0]); close(pipe_stdin[1]);
		throw std::runtime_error("fork() failed");
	}

	if (_pid == 0) {
		dup2(pipe_stdin[0], STDIN_FILENO);
		dup2(pipe_stdout[1], STDOUT_FILENO);
		close(pipe_stdin[0]); close(pipe_stdin[1]);
		close(pipe_stdout[0]); close(pipe_stdout[1]);

		size_t slash = path.rfind('/');
		std::string dir  = (slash == std::string::npos) ? "." : path.substr(0, slash);
		std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
		if (chdir(dir.c_str()) == -1)
			exit(1);

		char** envp = buildEnvp(path);
		char* av[] = { (char*)interpreter.c_str(), (char*)file.c_str(), NULL };
		execve(interpreter.c_str(), av, envp);
		exit(1);

	} else {
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);
		_write_fd = pipe_stdin[1];
		_read_fd = pipe_stdout[0];

		if (fcntl(_write_fd, F_SETFL, fcntl(_write_fd, F_GETFL) | O_NONBLOCK) == -1
			|| fcntl(_read_fd, F_SETFL, fcntl(_read_fd, F_GETFL) | O_NONBLOCK) == -1) {
			close(_write_fd);
			close(_read_fd);
			kill(_pid, SIGKILL);
			waitpid(_pid, NULL, 0);
			throw std::runtime_error("fcntl() failed: " + std::string(strerror(errno)));
		}

		fcntl(_write_fd, F_SETFD, FD_CLOEXEC);
		fcntl(_read_fd, F_SETFD, FD_CLOEXEC);
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
	std::string scriptPath = (pos != std::string::npos) ? _req.uri.substr(0, pos) : _req.uri;
	envp->push_back("SCRIPT_NAME=" + scriptPath);
	envp->push_back("QUERY_STRING=" + (pos != std::string::npos ? _req.uri.substr(pos + 1) : ""));
	envp->push_back("PATH_INFO=" + scriptPath);
	envp->push_back("REQUEST_URI=" + _req.uri);
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

