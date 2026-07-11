#include "../../includes/http/HttpParser.hpp"

HttpParser::HttpParser(const Server& server) : _errorCode(0), _state(R_HEADERS),
											   _bodyExcepted(0), _bodyReceived(0),
											   _maxBodySize(server.max_body_client),
											   _server(&server) {}



HttpParser::~HttpParser() {}



void HttpParser::runParsing(std::string& buffer, size_t n) {
	(void)n;
	// ne pas écraser une erreur déjà flaggée (ex: 413 détecté sur un chunk,
	// alors que le terminateur "0\r\n\r\n" arrive dans un recv() ultérieur)
	_req.error = (_errorCode != 0) ? _errorCode : 200;
    _buffer += buffer;

    if (_state == R_HEADERS) {
		_req.isMultipart = false;
        size_t pos = _buffer.find("\r\n\r\n");
        if (pos == std::string::npos) {
			// garde-fou : des headers sans fin ne doivent pas faire grossir
			// le buffer indéfiniment (résilience exigée par le sujet)
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

	if (_state == R_BODY) {
		size_t left = _bodyExcepted - _bodyReceived;
		size_t acceptable = std::min(left, _buffer.size());
		if (_errorCode == 413) {
			// corps trop gros : jeter les données au lieu de les
			// accumuler, pour ne pas exploser la mémoire
			_buffer.clear();
		} else {
			_body.append(_buffer, 0, acceptable);
			_buffer.erase(0, acceptable);
		}
		_bodyReceived = _body.size();
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


	const Location* loc = findLocation(_req.uri.substr(0, _req.uri.find('?')), *_server);
	if (loc)
		_maxBodySize = loc->max_body_client;

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

	if (_bodyExcepted > _maxBodySize)
		return setError(413);
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

		std::string sizeStr = _buffer.substr(0, pos);

		char* end = NULL;
		size_t size = std::strtol(sizeStr.c_str(), &end, 16);

		if (end == sizeStr.c_str())
			setError(400);

		while(*end == ' ' || *end == '\t')
			end++;

		if (*end != '\0')
			setError(400);

		if (size == 0) {
			if (_errorCode != 413 && _errorCode != 400) {
				if (_buffer.size() < pos + 4)
					return;
				_buffer.erase(0, pos + 4);
			}
			else
				setError(400);
			if (_req.isMultipart == true)
				getMp();
			if (_errorCode == 413 || _errorCode == 400)
				_state = ERROR;
			else
				_state = COMPLETE;
            return;
		}
		size_t needed = pos + 2 + size + 2;
		if (_buffer.size() < needed)
    		return;

		// compter le chunk UNIQUEMENT quand il est entier dans le buffer :
		// avant, l'incrément se faisait avant le check de complétude, donc un
		// chunk arrivant en plusieurs recv() était re-compté à chaque appel
		// (jusqu'à ~16x avec des chunks de 64k et un buffer de 4k) → faux 413
		_bodyReceived += size;
		if (_bodyReceived > _maxBodySize)
			setError(413);
		
		if (_errorCode != 413 && _errorCode != 400)
			_body.append(_buffer, pos + 2, size);
		_buffer.erase(0, needed);
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
	// la limite a pu être écrasée par celle d'une location : revenir au
	// défaut du serveur pour la prochaine requête keep-alive
	_maxBodySize = _server->max_body_client;
	_body.clear();
	_header.clear();
	_req = HttpRequest();
}

