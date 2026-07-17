#pragma once

#include <string>
#include <cstddef>

enum TokenType {
	TOK_WORD,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_SEMI,
	TOK_EOF
};

struct Token {
	TokenType	type;
	std::string	value;
	size_t		line;
	size_t		col;
};

