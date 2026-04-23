#pragma once

#include <string>
#include <cstddef>

enum TokenType {
	TOK_WORD,	// listen, GET, 8080, tout ce qui n'est pas un symbole
	TOK_LBRACE,	// {
	TOK_RBRACE,	// }
	TOK_SEMI,	// ;
	TOK_EOF		// evite de checker _pos < _token.size()
};

struct Token {
	TokenType	type;
	std::string	value;
	size_t		line;
	size_t		col;
};

