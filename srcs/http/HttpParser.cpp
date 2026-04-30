#include "../../includes/http/HttpParser.hpp"
#include <sstream>
#include <string>
#include <cstdlib>

HttpParser::HttpParser(size_t maxBodyClient) : _state(R_HEADERS), _bodyExcepted(0), _bodyReceived(0), _maxBodySize(maxBodyClient) {}

HttpParser::~HttpParser() {}

void HttpParser::runParsing(std::string& buffer, size_t n) {
	(void)n;
    _buffer += buffer;
    if (_state == R_HEADERS) {
        size_t pos = _buffer.find("\r\n\r\n");
        if (pos == std::string::npos) 
            return ;
        _header = _buffer.substr(0, pos);
		_buffer = _buffer.substr(pos + 4);
        _state = R_BODY;
        headerParser();
        _state = (_bodyExcepted > 0) ? R_BODY : COMPLETE;
    }

	if (_state == R_BODY) {
		_body += _buffer;
		_buffer.clear();
		_bodyReceived = _body.size();
		if (_bodyReceived > _bodyExcepted)
			_state = ERROR // 400 bad request
		else
			_state = COMPLETE;
	}
}

void HttpParser::headerParser() {
	std::istringstream	iss(_header);
	std::string			line;
	
	std::getline(iss, line);
	std::istringstream	firstLine(line);
	firstLine >> _req.method >> _req.uri >> _req.version;


	while(std::getline(iss, line) && line != "\r") {
		size_t sep = line.find(": ");
		_req.headers[line.substr(0, sep)] = line.substr(sep + 2);
	}

	std::map<std::string, std::string>::iterator it;
	for (it = _req.headers.begin(); it != _req.headers.end(); ++it) {
		if (it->first == "Content-Length")
			_bodyExcepted = strtoull(it->second.c_str(), NULL, 10);
	}

	if (_bodyExcepted > _maxBodySize)
		_state = ERROR;
	
}

HttpParser::State	HttpParser::getState() {
    return _state;
}
