#pragma once

#include "Token.hpp"
#include <vector>

// Lexer qui tokenize seulement : ;, {, }, word, et EOF.
class Lexer {
	public:
		Lexer(const std::string& input);
		std::vector<Token>	tokenize();

	private:
		std::string	_input;
		size_t		_pos;
		size_t		_line;	// débogage en ligne et colonne
		size_t		_col;	// débogage

		bool	atEnd();
		char	current();
		void	advance();
		Token	readQuote(char quote);
		Token	readWord();
		Token	makeToken(TokenType type, const std::string& value);
		void	skipWhitespacesAndComments();	
};

