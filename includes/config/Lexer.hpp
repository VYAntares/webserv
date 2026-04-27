#pragma once

#include "Token.hpp"
#include <vector>

class Lexer {
	public:
		Lexer(const std::string& input);
		std::vector<Token>	tokenize();

	private:
		std::string	_input;
		size_t		_pos;
		size_t		_line;
		size_t		_col;

		bool	atEnd();
		char	current();
		void	advance();
		Token	readQuote();
		Token	readWord();
		Token	makeToken(TokenType type, const std::string& val);
		void	skipWhitespacesAndComments();	
};

