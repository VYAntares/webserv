#include "../../includes/http/HttpParser.hpp"

HttpParser::HttpParser(const Server& server) : _errorCode(0), _state(R_HEADERS),
											   _bodyExcepted(0), _bodyReceived(0),
											   _maxBodySize(server.max_body_client),
											   _server(&server) {}

HttpParser::~HttpParser() {}

void HttpParser::runParsing(std::string& buffer) {
	_req.error = (_errorCode != 0) ? _errorCode : 200;
	_buffer += buffer;

	State before;
	do {
		before = _state;
		switch (_state) {
			case R_HEADERS: treatHeader();	break;
			case R_BODY:	treatBody();	break;
			case R_CHUNKED:	readChunked();	break;
			default:		break;
		}
	} while (_state != before && _state != COMPLETE && _state != ERROR);
}

void HttpParser::treatHeader() {
	_req.isMultipart = false;
 	size_t pos = _buffer.find("\r\n\r\n");
	if (pos == std::string::npos) {
		if (_buffer.size() > MAX_HEADER_SIZE)
			setError(431);
		return ;
	}
	_header = _buffer.substr(0, pos);
	_buffer = _buffer.substr(pos + 4);
	_state = R_BODY;
	headerParser();
	if (_state == ERROR)
		return;
	if (_state != R_CHUNKED)
	   	_state = (_bodyExcepted > 0) ? R_BODY : COMPLETE;
}

void HttpParser::treatBody() {
	size_t left = _bodyExcepted - _bodyReceived;
	size_t acceptable = std::min(left, _buffer.size());
	if (_errorCode == 413) {
		_buffer.erase(0, acceptable);
	} else {
		_body.append(_buffer, 0, acceptable);
		_buffer.erase(0, acceptable);
	}
	_bodyReceived += acceptable;
	if (_bodyReceived == _bodyExcepted) {
		_req.error = (_errorCode != 0) ? _errorCode : 200;
		if (_errorCode == 0) {
			_req.body = _body;
			if (_req.isMultipart == true)
				getMp();
		}
		_state = (_errorCode == 0) ? COMPLETE : ERROR;
	}
}

void HttpParser::setError(int errorCode) {
	_req.error = errorCode;
	_errorCode = errorCode;
	if (_state != R_CHUNKED && errorCode != 413)
		_state = ERROR;
}

void HttpParser::getMp() {
	MultipartParser mp(_req.boundary, _req.body);
	_req.mp = mp.parsePart();
}

void HttpParser::checkFirstLine() {
	if (_req.method.empty() || _req.method.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos)
		return setError(400);

	if (_req.uri.empty() || _req.uri[0] != '/') 
		return setError(400);

	if (isEncoded(_req.uri))
		_req.uri = decodeHexa(_req.uri, 0);

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

	while (std::getline(iss, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			break;
		size_t sep = line.find(": ");
		if (sep == std::string::npos)
			return setError(400);
		_req.headers[line.substr(0, sep)] = line.substr(sep + 2);
	}

	const Location* loc = findLocation(_req.uri.substr(0, _req.uri.find('?')), *_server);
	if (loc)
		_maxBodySize = loc->max_body_client;

	findHeaders();
	if (_bodyExcepted > _maxBodySize)
		return setError(413);
}

void HttpParser::findHeaders() {
	std::map<std::string, std::string>::iterator it;
	for (it = _req.headers.begin(); it != _req.headers.end(); ++it) {
		if (it->first == "Content-Length") {
			std::string& cl = it->second;
			if (cl.find_first_not_of("0123456789") != std::string::npos)
				return setError(400);
			_bodyExcepted = strtoull(cl.c_str(), NULL, 10);
		}
		if (it->first == "Transfer-Encoding" && it->second.find("chunked") != std::string::npos)
			_state = R_CHUNKED;
		if (it->second.find("multipart/form-data") != std::string::npos)
			setBoundary(it->second);
	}
}

void HttpParser::setBoundary(const std::string& boundary) {
	size_t pos = boundary.find("boundary=");

	if (pos != std::string::npos) {
		for (size_t i = pos + 9; i < boundary.length(); i++)
			_req.boundary += boundary[i];
	}

	_req.isMultipart = true;
}

void HttpParser::readChunked() {
	while (true) {

		size_t pos = _buffer.find("\r\n");
		if (pos == std::string::npos)
			return ;

		char* 		end 	= NULL;
		std::string sizeStr = _buffer.substr(0, pos);
		size_t 		size 	= std::strtol(sizeStr.c_str(), &end, 16);

		if (end == sizeStr.c_str())
			setError(400);
		while(*end == ' ' || *end == '\t')
			end++;
		if (*end != '\0' && *end != ';')
			setError(400);

		if (size == 0) {
			endOfChunked(pos);
			return ;
		}

		size_t needed = pos + 2 + size + 2;
		if (_buffer.size() < needed)
			return;

		_bodyReceived += size;
		if (_bodyReceived > _maxBodySize) {
			setError(413);
		}
		
		if (_errorCode != 413 && _errorCode != 400)
			_body.append(_buffer, pos + 2, size);
		_buffer.erase(0, needed);
	}
}

void HttpParser::endOfChunked(size_t pos) {
	if (_errorCode != 413 && _errorCode != 400) {
		if (_buffer.size() < pos + 4)
			return;
		_buffer.erase(0, pos + 4);
	}
	_req.body = _body;
	if (_req.isMultipart == true)
		getMp();
	if (_errorCode == 413 || _errorCode == 400)
		_state = ERROR;
	else
		_state = COMPLETE;
	return ;
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
	_maxBodySize = _server->max_body_client;
	_body.clear();
	_header.clear();
	_req = HttpRequest();
}

