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

void CGIReadHandler::reapChild(int& status) {
	pid_t r = waitpid(_pid, &status, WNOHANG);
	if (r == 0) {
		kill(_pid, SIGKILL);
		waitpid(_pid, &status, 0);
		status = 0;
	}
	_reaped = true;
}

std::string CGIReadHandler::parseCgi() {
	int			code = 200;
	std::string	type;
	std::string	body;
	std::string	cookie;

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
			else if (line.compare(0, 11, "Set-Cookie:") == 0) {
				size_t v = 11;
				while (v < line.size() && line[v] == ' ')
					v++;
				cookie = line.substr(v);
			}
		}
	}
	if (code <= 0)
		code = 200;

	StaticHandler st(*_loc, code, body);
	if (!type.empty())
		st.setType(type);
	if(!cookie.empty())
		st.setCookie(cookie);

	return st.buildResponse();
}

int CGIReadHandler::handle_input() {
	_lastActivity = time(NULL);

	char buf[4096];
	ssize_t n = read(_fd, buf, sizeof(buf));

	if (n > 0) {
		_out.append(buf, n);
		return 0;
	}
	if (n < 0) {
		if (_sink) {
			ErrorHandler err(*_loc, 502, "");
			_sink->onCgiDone(err.buildResponse());
			_responded = true;
		}
		return -1;
	}

	std::cerr << "[DEBUG CGIReadHandler EOF] _out.size()=" << _out.size() << std::endl;
	int status = 0;
	reapChild(status);
	std::cerr << "[DEBUG CGIReadHandler status] WIFEXITED=" << WIFEXITED(status)
			  << " WEXITSTATUS=" << (WIFEXITED(status) ? WEXITSTATUS(status) : -1)
			  << " WIFSIGNALED=" << WIFSIGNALED(status) << std::endl;

	if (!_sink)
		return -1;

	std::string raw;
	if (WIFSIGNALED(status)) {
		ErrorHandler err(*_loc, 502, "");
		raw = err.buildResponse();
	}
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

	return -1;
}

CGIReadHandler::~CGIReadHandler() {
	close(_fd);
	if (!_reaped) {
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, 0);
	}
	if (_sink && !_responded) {
		ErrorHandler err(*_loc, 504, "");
		_sink->onCgiDone(err.buildResponse());
	}
}
