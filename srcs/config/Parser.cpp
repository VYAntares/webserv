#include "../../includes/config/Parser.hpp"

#include "iostream"
#include "sstream"
#include <typeinfo>

// entier vers string
static std::string toString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}

Parser::Parser(const std::vector<Token>& tokens) : _pos(0) , _tokens(tokens){
}

// return le token actuel
Token	Parser::current() { return _tokens[_pos]; }

// consomme et avance de token
Token	Parser::consume() { return _tokens[_pos++]; }

// si ce n'est pas le bon token attendu -> error
// sinon on consomme
Token	Parser::except(TokenType type) {
	if (_tokens[_pos].type != type)	
		throw std::runtime_error("unexpected token at line "
								+ toString(_tokens[_pos].line));
	return _tokens[_pos++];
}

// lance le parsing de chaque bloc serveur
void	Parser::parse() {
	while (current().type != TOK_EOF) {
		if (current().value == "server")
			parseServerBlock();
		else
			throw std::runtime_error("excepted 'server' at line: "
									+ toString(_tokens[_pos].line));
	}
}

// parse le bloc serveur, en appellant le parser de location 
// ou on parse une directive, et si on arrive a la fin sans trouver
// de bracket fermante, throw erreur
void	Parser::parseServerBlock() {
	Server s;

	std::cout << ">>> SERVER block (ligne " << current().line << ")" << std::endl;	
	consume();
	except(TOK_LBRACE);
	
	while (current().type != TOK_RBRACE) {
		if (current().value == "location")
			parseLocationBlock(s);
		else if (current().type == TOK_WORD)
			parseDirective(s);
		else if (current().type == TOK_EOF)
			throw std::runtime_error("unclosed server block, expected '}' at line "
            						+ toString(current().line));
	}

	except(TOK_RBRACE);
	_c.cfg.push_back(s);
}

// on parse les locations, et on les lie au bloc serveur
void	Parser::parseLocationBlock(Server& s) {
	Location	l;

	consume();
	l.path = except(TOK_WORD).value;
	std::cout << "  >>> LOCATION '" << l.path << "' (ligne " << current().line << ")" << std::endl;
	except(TOK_LBRACE);

	while (current().type != TOK_RBRACE) {
		if (current().type == TOK_WORD)
			parseDirective(l);
		else if (current().type == TOK_EOF)
			throw std::runtime_error("unclosed location block, expected '}' at line "
            						+ toString(current().line));
	
	}
	
	except(TOK_RBRACE);
	s.locations.push_back(l);
}

// parsing de directives, 
template<typename T>
void	Parser::parseDirective(T& block) {
	(void)block;
	std::string context = (typeid(block) == typeid(Server)) ? "SERVER" : "LOCATION";
	std::string key = consume().value;
	std::cout << "  [" << context << "] directive= '" << key << "' valeurs: ";
	while (current().type != TOK_SEMI && current().type != TOK_EOF)
		std::cout << "'" << consume().value << "' ";
	std::cout << std::endl;
	except(TOK_SEMI);
}

template void Parser::parseDirective<Server>(Server&);
template void Parser::parseDirective<Location>(Location&);

