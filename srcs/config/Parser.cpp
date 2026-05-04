#include "../../includes/config/Parser.hpp"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <netdb.h>
#include <netinet/in.h>

// =======================================================
// ||                       UTILS                       ||
// =======================================================



// return le token actuel
Token	Parser::current() { return _tokens[_pos]; }



// consomme et avance de token
Token	Parser::consume() { return _tokens[_pos++]; }



// entier vers string
std::string toString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}



static std::string tokenTypeName(TokenType t) {
	switch (t) {
		case TOK_WORD:   return "word";
		case TOK_LBRACE: return "'{'";
		case TOK_RBRACE: return "'}'";
		case TOK_SEMI:   return "';'";
		case TOK_EOF:    return "EOF";
		default:         return "unknown";
	}
}



// si ce n'est pas le bon token attendu -> erreur
// sinon on le consomme
Token	Parser::expect(TokenType type) {
	if (_tokens[_pos].type != type)
		throw std::runtime_error("expected " + tokenTypeName(type)
			+ " but got '" + _tokens[_pos].value
			+ "' at line " + toString(_tokens[_pos].line));
	return _tokens[_pos++];
}



// =======================================================
// ||                       COEUR                       ||
// =======================================================
//
// ================ Constructeur
//
//
Parser::Parser(const std::vector<Token>& tokens) : _pos(0) , _tokens(tokens){
}



// ================ parse - entry point
//
//
// lance le parsing de chaque bloc serveur
void	Parser::parse() {
	while (current().type != TOK_EOF) {
		if (current().value == "server")
			parseServerBlock();
		else
			throw std::runtime_error("expected 'server' at line: "
									+ toString(_tokens[_pos].line));
	}
}



// ================ ParseServerBlock
//
//
// parse un bloc server : appelle le parser de location ou parse une directive
// lève une erreur si le bloc n'est pas fermé par '}'
void	Parser::parseServerBlock() {
	Server s;

	consume();
	expect(TOK_LBRACE);
	
	while (current().type != TOK_RBRACE) {
		if (current().value == "location")
			parseLocationBlock(s);
		else if (current().type == TOK_WORD)
			parseDirective(s);
		// si on arrive à la fin du fichier sans trouver '}', le bloc n'est pas fermé
		else if (current().type == TOK_EOF)
			throw std::runtime_error("unclosed server block, expected '}' at line "
            						+ toString(current().line));
		// tout autre token est inattendu à cet endroit
		else
			throw std::runtime_error("unexpected token '" + current().value
									+ "' at line " + toString(current().line));
	}
	expect(TOK_RBRACE);

	// initialisation de max_body_client a 1MB par defaut
	if (s.max_body_client == SIZE_MAX)
		s.max_body_client = 1024 * 1024;
	_c.cfg.push_back(s);
}



// ================ ParseLocationBlock
//
//
// on parse les locations, et on les lie au bloc serveur
void	Parser::parseLocationBlock(Server& s) {
	Location	l;

	consume();
	l.path = expect(TOK_WORD).value;
	expect(TOK_LBRACE);

	while (current().type != TOK_RBRACE) {
		if (current().type == TOK_WORD)
			parseDirective(l);
		// si on arrive à la fin du fichier sans trouver '}', le bloc n'est pas fermé
		else if (current().type == TOK_EOF)
			throw std::runtime_error("unclosed location block, expected '}' at line "
            						+ toString(current().line));
		// tout autre token est inattendu à cet endroit
		else
			throw std::runtime_error("unexpected token '" + current().value + "' at line "
									+ toString(current().line));
	}
	expect(TOK_RBRACE);
	s.locations.push_back(l);
}



// =======================================================
// ||                    DIRECTIVES                     ||
// =======================================================
//
// ================ parseDirective Server
//
//
// parse les directives propres au bloc server
void	Parser::parseDirective(Server& s) {
	std::string key = consume().value;

	// listen et server_name sont propres au bloc server
	if (key == "listen") {
		s.listen.push_back(parseListen());
		while (current().type == TOK_WORD)
			s.listen.push_back(parseListen());
	}
	else if (key == "server_name") {
		s.server_name.push_back(expect(TOK_WORD).value);
		while (current().type == TOK_WORD)
			s.server_name.push_back(expect(TOK_WORD).value);
	}
	// directive appartenant uniquement au bloc location -> erreur
	else if (key == "upload_store" || key == "cgi_pass" || key == "allowed_methods")
		throw std::runtime_error("directive '" + key
									+ "' is location-only, not allowed in server at line "
									+ toString(current().line));
	// directive commune avec location
	else
		putDirective(static_cast<BaseBlock&>(s), key);

	// toute directive doit se terminer par un ';'
	if (current().type != TOK_SEMI)
		throw std::runtime_error("missing ';' after directive '"
			+ key + "' at line " + toString(current().line));
	expect(TOK_SEMI);
}



// ================ parseDirective Location
//
//
// parser de directive location
// upload_store, cgi_pass, allowed_methodes
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
	// directive appartenant uniquement au bloc server -> erreur
	else if (key == "listen" || key == "server_name")
		throw std::runtime_error("directive '" + key
									+ "' is server-only, not allowed in location at line "
									+ toString(current().line));
	// directive commune avec server
	else
		putDirective(static_cast<BaseBlock&>(l), key);

	// toute directive doit se terminer par un ';'
	if (current().type != TOK_SEMI)
		throw std::runtime_error("missing ';' after directive '"
			+ key + "' at line " + toString(current().line));
	expect(TOK_SEMI);
}



// ================ putDirective (block)
//
//
// parse les directives communes aux blocs server et location
// root, index, error_page, autoindex, return, client_max_body_size
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
	// parsing spécifique à client_max_body_size
	else if (key == "client_max_body_size") {
		// initialisé a SIZE_MAX de base, si l'on parse, ne sera plus SIZE_MAX
		if (b.max_body_client != SIZE_MAX)
			throw std::runtime_error("'client_max_body_size' directive is duplicate at line "
										+ toString(current().line));
		b.max_body_client = parseClientBody();
	}
	// directive non reconnue -> erreur
	else
		throw std::runtime_error("directive '" + key + "' is not allowed here at line "
									+ toString(current().line));
}



// =======================================================
// ||                  "SPECIAL CASE"                   ||
// =======================================================

// ================ error_page
//
//
// format : error_page 500 501 /pages/error.html
// le dernier token est le path, tous les précédents sont des codes
void	Parser::parseErrorPage(BaseBlock& b) {
	std::vector<std::string> args;

	size_t line = current().line;
	while (current().type == TOK_WORD)
		args.push_back(expect(TOK_WORD).value);
	// error_page necessite 2 argument minimums. 
	if (args.size() < 2)
		throw std::runtime_error("error_page requires at least one code and a path at line "
           						 + toString(line));

	// ajout, parsing et verification des code, doit etre digit et entre 300 et 599
	std::string path = args.back();
	for (size_t i = 0; i < args.size() - 1; i++) {
		char* 	end;
		long	code = std::strtol(args[i].c_str(), &end, 10);
		if (*end != '\0' || code < 300 || code > 599)
            throw std::runtime_error("invalid error code '" + args[i] + "' in error_page");
		b.error_page[(int)code] = path;
	}
}



// ================ autoindex
//
//
// n'accepte que 'on' (1) ou 'off' (0), toute autre valeur est une erreur
int		Parser::parseAutoindex() {
	Token tok = consume();
	if (tok.value == "on")  return 1;
	if (tok.value == "off") return 0;
	throw std::runtime_error("invalid value '" + tok.value + "' for 'autoindex'"
							+ ", must be 'on' or 'off' at line " + toString(tok.line));
}



// ================ return
//
//
// format : "return 500" ou "return 500 /path/vers/quelquepart"
std::pair<int, std::string>	Parser::parseReturn() {
	Token	tok = consume();

	char*	end;
	long	code = std::strtol(tok.value.c_str(), &end, 10);

	// si code a autre chose que digit (ou en dehors des erreurs html)
	if (*end != '\0' || code < 0 || code > 999)
		throw std::runtime_error("invalid return code '" + tok.value
								+ "' at line " + toString(tok.line));

	// si il y a ";" apres le code, alors path = vide
	std::string path = (current().type != TOK_SEMI) ? expect(TOK_WORD).value : "";
	return std::make_pair((int)code, path);
}



// ================ listen
//
//
// vérifie que l'adresse et le port sont valides avant de les insérer dans les structs
addrport	Parser::parseListen() {
	// lit le token de la valeur listen (ex: "127.0.0.1:8080" ou "8080")
	Token		tok = expect(TOK_WORD);
	std::string val = tok.value;
	size_t		colon = val.find(':');

	// sépare host et port ; si pas de ':', le token est uniquement un port
	// et on écoute sur toutes les interfaces
	std::string	host = (colon != std::string::npos) ? val.substr(0, colon) : "0.0.0.0";
	std::string	port = (colon != std::string::npos) ? val.substr(colon + 1) : val;

	// valide le port avant getaddrinfo : strtol détecte les non-numériques,
	// et le range check attrape les valeurs hors 1-65535 que getaddrinfo laisserait passer
	char*	end;
	long	p = std::strtol(port.c_str(), &end, 10);
	if (*end != '\0' || p < 1 || p > 65535)
		throw std::runtime_error("invalid port '" + port + "' at line " + toString(tok.line));

	// hints contraint getaddrinfo à retourner uniquement IPv4 + TCP
	// sans hints, elle pourrait retourner IPv6 ou UDP, rendant le cast sockaddr_in* invalide
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family		= AF_INET;
	hints.ai_socktype	= SOCK_STREAM;

	// getaddrinfo() prend le string "host" et remplit une liste chaînée de struct addrinfo
	// res pointe sur le premier résultat ; grâce aux hints, c'est garanti une sockaddr_in
	struct addrinfo* res = NULL;
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
		throw std::runtime_error("invalid address '" + host + "' at line " + toString(tok.line));

	// res->ai_addr est un sockaddr* générique → cast en sockaddr_in* (safe grâce aux hints)
	// sin_addr.s_addr extrait le uint32_t big-endian (ex: "127.0.0.1" → 0x0100007F)
	uint32_t addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
	freeaddrinfo(res); // libère toute la liste chaînée allouée par getaddrinfo

	return std::make_pair(addr, (int)p);
}



// ================ client_max_body_size
//
//
// parse client_max_body_size et convertit directement les unités (k, m, g) en octets
size_t	Parser::parseClientBody() {
	Token		tok = expect(TOK_WORD);
	std::string	val = tok.value;

	size_t i = 0;
	while (i < val.length() && std::isdigit(val[i]))
		i++;
	if (i == 0)
		throw std::runtime_error("'client_max_body_size' directive invalid value at line "
								+ toString(tok.line));

	// extraction de la valeur numérique et de l'unité (kilo, mega, giga)
	char*			end;
	unsigned long 	num = std::strtoul(val.substr(0, i).c_str(), &end, 10);
	if (*end != '\0')
		throw std::runtime_error("'client_max_body_size' invalid numeric value '" 
								+ val.substr(0, i) + "' at line " + toString(tok.line));

	std::string	unit = val.substr(i);
	for (size_t j = 0; j < unit.length(); j++)
		unit[j] = std::tolower(unit[j]);

	// conversion selon l'unité
	if (unit.empty())   return num;
	if (unit == "k")    return num * 1024;
	if (unit == "m")    return num * 1024 * 1024;
	if (unit == "g")    return num * 1024 * 1024 * 1024;
	throw std::runtime_error("'client_max_body_size' invalid value at line "
							+ toString(tok.line));
}

