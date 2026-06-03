#include "../../includes/http/HttpParser.hpp"
#include <sstream>
#include <string>
#include <cstdlib>

HttpParser::HttpParser(size_t maxBodyClient) : _errorCode(0), _state(R_HEADERS), _bodyExcepted(0), _bodyReceived(0), _maxBodySize(maxBodyClient) {}

HttpParser::~HttpParser() {}

void HttpParser::runParsing(std::string& buffer, size_t n) {
	(void)n;
	_req.error = 200;
    _buffer += buffer;

    if (_state == R_HEADERS) {
        size_t pos = _buffer.find("\r\n\r\n");
        if (pos == std::string::npos) 
            return ;
        _header = _buffer.substr(0, pos);
		_buffer = _buffer.substr(pos + 4);
        _state = R_BODY;
        headerParser();
		if (_state == ERROR)
			return;
		if (_state != R_CHUNKED)
        	_state = (_bodyExcepted > 0) ? R_BODY : COMPLETE;
    }

	if (_state == R_BODY) {
		_body += _buffer;
		_buffer.clear();
		_bodyReceived = _body.size();
		if (_bodyReceived > _bodyExcepted)
			setError(400);
		else if (_bodyReceived == _bodyExcepted) {
			_req.error = 200;
			_req.body = _body;
			_state = COMPLETE;
		}
	}
	if (_state == R_CHUNKED)
		readChunked();
	if (_state == ERROR)
		return;
}

void HttpParser::setError(int errorCode) {
	_req.error = errorCode; 
	_errorCode = errorCode;
	if (_state != R_CHUNKED)
		_state = ERROR;
}

void HttpParser::checkFirstLine() {
	if (_req.method.empty() || _req.method.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos)
		return setError(400);

	if (_req.uri.empty() || _req.uri[0] != '/') 
		return setError(400);

	if (_req.version != "HTTP/1.1" && _req.version != "HTTP/1.0") 
		return setError(400);
}

void HttpParser::headerParser() {
	std::istringstream	iss(_header);
	std::string			line;
	
	std::getline(iss, line);
	std::istringstream	firstLine(line);
	firstLine >> _req.method >> _req.uri >> _req.version;

	checkFirstLine();

	while(std::getline(iss, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			break;

		size_t sep = line.find(": ");
		if (sep == std::string::npos)
			return setError(400);
		_req.headers[line.substr(0, sep)] = line.substr(sep + 2);
	}

	std::map<std::string, std::string>::iterator it;
	for (it = _req.headers.begin(); it != _req.headers.end(); ++it) {
		if (it->first == "Content-Length") {
			std::string& cl = it->second;
			if (cl.find_first_not_of("0123456789") != std::string::npos)
				return setError(400);
			_bodyExcepted = strtoull(cl.c_str(), NULL, 10);
		}
		if (it->second == "chunked")
			_state = R_CHUNKED;
	}

	if (_bodyExcepted > _maxBodySize)
		return setError(413);
}

void HttpParser::readChunked() {
	while (true) {

		size_t pos = _buffer.find("\r\n");
		if (pos == std::string::npos)
			return ;

		std::string sizeStr = _buffer.substr(0, pos);

		char* end = NULL;
		size_t size = std::strtol(sizeStr.c_str(), &end, 16);

		if (end == sizeStr.c_str())
			return setError(400);

		while(*end == ' ' || *end == '\t')
			(*end)++;

		if (*end != '\0')
			return setError(400);

		_bodyReceived += size;
		if (_bodyReceived > _maxBodySize)
			return setError(413);

		if (size == 0) {
			if (_errorCode == 413 || _errorCode == 400)
				_state = ERROR;
			else
				_state = COMPLETE;
            _req.body = _body;
            return;
		}
		size_t needed = pos + 2 + size + 2;
		if (_buffer.size() < needed)
    		return;

		if (_errorCode != 413 && _errorCode != 400) {
			_body.append(_buffer, pos + 2, size);
			_buffer.erase(0, needed);			
		}
	}
}

HttpParser::State	HttpParser::getState() {
    return _state;
}

HttpRequest	HttpParser::getReq() {
    return _req;
}

void HttpParser::reset() {
	_errorCode = 0;
	_state = R_HEADERS;
	_bodyExcepted = 0;
	_bodyReceived = 0;
}
