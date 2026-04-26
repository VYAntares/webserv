#include "../../includes/config/Lexer.hpp"

Lexer::Lexer(const std::string& input) : _input(input), _pos(0), _line(1), _col(1) {}

bool	Lexer::atEnd() { return _pos >= _input.size(); }

char	Lexer::current() { return _input[_pos]; }

void	Lexer::skipWhitespacesAndComments() {

}

std::vector<Token> Lexer::tokenize() {
	std::vector<Token> token;
	while (!atEnd()) {
		skipWhitespacesAndComments();
		if (atEnd()) {
			// former token OEF
		}
	}
	return token;
}

