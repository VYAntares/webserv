#include "../../includes/config/Parser.hpp"

#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <netdb.h>
#include <netinet/in.h>

Token	Parser::current() { return _tokens[_pos]; }

Token	Parser::consume() { return _tokens[_pos++]; }

static std::string	tokenTypeName(TokenType t) {
	switch (t) {
		case TOK_WORD:   return "word";
		case TOK_LBRACE: return "'{'";
		case TOK_RBRACE: return "'}'";
		case TOK_SEMI:   return "';'";
		case TOK_EOF:    return "EOF";
		default:         return "unknown";
	}
}

Token	Parser::expect(TokenType type) {
	if (_tokens[_pos].type != type)
		throw std::runtime_error("expected " + tokenTypeName(type)
								+ " but got '" + _tokens[_pos].value
								+ "' at line " + toString(_tokens[_pos].line));
	return _tokens[_pos++];
}

Parser::Parser(const std::vector<Token>& tokens) : _pos(0) , _tokens(tokens){
}

Config	Parser::parse() {
	while (current().type != TOK_EOF) {
		if (current().value == "server")
			parseServerBlock();
		else
			throw std::runtime_error("expected 'server' at line: "
									+ toString(_tokens[_pos].line));
	}
	return _c;
}

void	Parser::parseServerBlock() {
	Server s;

	consume();
	expect(TOK_LBRACE);
	
	while (current().type != TOK_RBRACE) {
		if (current().value == "location")
			parseLocationBlock(s);
		else if (current().type == TOK_WORD)
			parseDirective(s);
		else if (current().type == TOK_EOF)
			throw std::runtime_error("unclosed server block, expected '}' at line "
            						+ toString(current().line));
		else
			throw std::runtime_error("unexpected token '" + current().value
									+ "' at line " + toString(current().line));
	}
	expect(TOK_RBRACE);

	if (s.max_body_client == SIZE_MAX)
		s.max_body_client = 1024 * 1024;
	_c.cfg.push_back(s);
}

void	Parser::parseLocationBlock(Server& s) {
	Location	l;

	consume();
	l.path = expect(TOK_WORD).value;

	for (size_t i = 0; i < s.locations.size(); i++) {
		if (l.path == s.locations[i].path)
			throw std::runtime_error("duplicate location ' " + l.path
									+ "' at line " + toString(current().line));
	}

	expect(TOK_LBRACE);
	while (current().type != TOK_RBRACE) {
		if (current().type == TOK_WORD)
			parseDirective(l);
		else if (current().type == TOK_EOF)
			throw std::runtime_error("unclosed location block, expected '}' at line "
            						+ toString(current().line));
		else
			throw std::runtime_error("unexpected token '" + current().value + "' at line "
									+ toString(current().line));
	}
	expect(TOK_RBRACE);
	s.locations.push_back(l);
}

void	Parser::parseDirective(Server& s) {
	std::string key = consume().value;

	if (key == "listen") {
		s.listen.push_back(parseListen(s));
		while (current().type == TOK_WORD)
			s.listen.push_back(parseListen(s));
	}
	else if (key == "server_name") {
		if (current().type != TOK_WORD)
			throw std::runtime_error("invalid number of arguments in 'server_name' directive at line "
									+ toString(current().line));
		while (current().type == TOK_WORD)
			s.server_name.push_back(consume().value);
	}
	else if (key == "upload_store" || key == "cgi_pass" || key == "allowed_methods")
		throw std::runtime_error("directive '" + key
								+ "' is location-only, not allowed in server at line "
								+ toString(current().line));
	else
		putDirective(static_cast<BaseBlock&>(s), key);

	if (current().type != TOK_SEMI)
		throw std::runtime_error("missing ';' after directive '"
			+ key + "' at line " + toString(current().line));
	expect(TOK_SEMI);
}

void	Parser::parseDirective(Location& l) {
	std::string key = consume().value;

	if (key == "upload_store")
		l.upload_store = expect(TOK_WORD).value;
	else if (key == "cgi_pass") {
		std::string ext = expect(TOK_WORD).value;
		l.cgi_pass[ext] = expect(TOK_WORD).value;
	}
	else if (key == "allowed_methods") {
		l.allowed_methods.push_back(expect(TOK_WORD).value);
		while (current().type == TOK_WORD)
			l.allowed_methods.push_back(expect(TOK_WORD).value);
	}
	else if (key == "listen" || key == "server_name")
		throw std::runtime_error("directive '" + key
								+ "' is server-only, not allowed in location at line "
								+ toString(current().line));
	else
		putDirective(static_cast<BaseBlock&>(l), key);

	if (current().type != TOK_SEMI)
		throw std::runtime_error("missing ';' after directive '" + key
								+ "' at line " + toString(current().line));
	expect(TOK_SEMI);
}

void	Parser::putDirective(BaseBlock& b, const std::string& key) {
	if (key == "error_page")
		parseErrorPage(b);
	else if (key == "return")
		b.return_path = parseReturn();
	else if (key == "root") {
		if (!b.root.empty())
			throw std::runtime_error("'root' directive is duplicate at line "
									+ toString(current().line));
		b.root = expect(TOK_WORD).value;
	}
	else if (key == "index") {
		if (!b.index.empty())
			throw std::runtime_error("'index' directive is duplicate at line "
									+ toString(current().line));
		b.index = expect(TOK_WORD).value;
	}
	else if (key == "autoindex") {
		if (b.autoindex != -1) 
			throw std::runtime_error("'autoindex' directive is duplicate at line "
									+ toString(current().line));
		b.autoindex = parseAutoindex();
	}
	else if (key == "client_max_body_size") {
		if (b.max_body_client != SIZE_MAX)
			throw std::runtime_error("'client_max_body_size' directive is duplicate at line "
									+ toString(current().line));
		b.max_body_client = parseClientBody();
	}
	else
		throw std::runtime_error("directive '" + key + "' is not allowed here at line "
								+ toString(current().line));
}

void	Parser::parseErrorPage(BaseBlock& b) {
	std::vector<std::string> args;

	size_t line = current().line;
	while (current().type == TOK_WORD)
		args.push_back(expect(TOK_WORD).value);
	if (args.size() < 2)
		throw std::runtime_error("error_page requires at least one code and a path at line "
           						 + toString(line));

	std::string path = args.back();
	for (size_t i = 0; i < args.size() - 1; i++) {
		char* 	end;
		long	code = std::strtol(args[i].c_str(), &end, 10);
		if (*end != '\0' || code < 400 || code > 599)
            throw std::runtime_error("invalid error code '" + args[i] + "' in error_page");
		b.error_page[(int)code] = path;
	}
}

int		Parser::parseAutoindex() {
	Token tok = consume();
	if (tok.value == "on")  return 1;
	if (tok.value == "off") return 0;
	throw std::runtime_error("invalid value '" + tok.value + "' for 'autoindex'"
							+ ", must be 'on' or 'off' at line " + toString(tok.line));
}

std::pair<int, str>	Parser::parseReturn() {
	Token	tok = consume();

	char*	end;
	long	code = std::strtol(tok.value.c_str(), &end, 10);

	if (*end != '\0' || code < 0 || code > 999)
		throw std::runtime_error("invalid return code '" + tok.value
								+ "' at line " + toString(tok.line));

	std::string path = (current().type != TOK_SEMI) ? expect(TOK_WORD).value : "";
	return std::make_pair((int)code, path);
}

addrport	Parser::parseListen(Server& s) {
	std::string val = expect(TOK_WORD).value;
	std::string host;
	std::string port;

	splitHostAndPort(host, port, val);

	char*	end;
	long	p = std::strtol(port.c_str(), &end, 10);
	if (*end != '\0' || p < 1 || p > 65535)
		throw std::runtime_error("invalid port '" + port
								+ "' at line " + toString(current().line));

	uint32_t addr = resolveHost(host, port);

	for (size_t i = 0; i < s.listen.size(); i++) {
		if (s.listen[i].first == addr && s.listen[i].second == p)
			throw std::runtime_error("a duplicate listen '" + addrToStr(addr) + ":"
									+ toString((size_t)p) + "' at line "
									+ toString(current().line));
	}
	return std::make_pair(addr, (int)p);
}

void	Parser::splitHostAndPort(std::string& host, std::string& port, std::string& val) {
	size_t		colon = val.find(':');
	if (colon != std::string::npos) {
		host = val.substr(0, colon);
		port = val.substr(colon + 1);
	} else {
		char* e;
		long  n = std::strtol(val.c_str(), &e, 10);
		if (*e == '\0') {
			if (n >= 1 && n <= 65535) {
				host = "0.0.0.0";
				port = val;
			} else
				throw std::runtime_error("invalid port '" + val + "'");
		} else {
			host = val;
			port = "8080";
		}
	}
	if (host == "*")
		host = "0.0.0.0";
}

uint32_t	Parser::resolveHost(std::string& host, std::string& port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family		= AF_INET;
	hints.ai_socktype	= SOCK_STREAM;

	struct addrinfo* res = NULL;
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
		throw std::runtime_error("invalid address '" + host
								+ "' at line " + toString(current().line));

	uint32_t addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
	freeaddrinfo(res);

	return addr;
}

size_t	Parser::parseClientBody() {
	Token		tok = expect(TOK_WORD);
	std::string	val = tok.value;

	size_t i = 0;
	while (i < val.length() && std::isdigit(val[i]))
		i++;
	if (i == 0)
		throw std::runtime_error("'client_max_body_size' directive invalid value at line "
								+ toString(tok.line));

	char*			end;
	unsigned long 	num = std::strtoul(val.substr(0, i).c_str(), &end, 10);
	if (*end != '\0')
		throw std::runtime_error("'client_max_body_size' invalid numeric value '"
								+ val.substr(0, i) + "' at line " + toString(tok.line));

	std::string	unit = val.substr(i);
	for (size_t j = 0; j < unit.length(); j++)
		unit[j] = std::tolower(unit[j]);

	if (unit.empty())   return num;
	if (unit == "k")    return num * 1024;
	if (unit == "m")    return num * 1024 * 1024;
	if (unit == "g")    return num * 1024 * 1024 * 1024;
	throw std::runtime_error("'client_max_body_size' invalid value at line "
							+ toString(tok.line));
}

