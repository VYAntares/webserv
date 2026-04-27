#include "../../includes/config/Lexer.hpp"

Lexer::Lexer(const std::string& input) : _input(input), _pos(0), _line(1), _col(1) {}

bool	Lexer::atEnd() { return _pos >= _input.size(); }

char	Lexer::current() { return _input[_pos]; }

void	Lexer::advance() {
	if (current() == '\n')	{ _line++; _col = 1; }
	else					{ _col++; }
	_pos++;
}

void	Lexer::skipWhitespacesAndComments() {
	while (!atEnd()) {
		char c = current();
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			advance();
		else if (c == '#') {
			while (!atEnd() && current() != '\n')
				advance();
		} else 
			break;
	}
}

Token	Lexer::makeToken(TokenType type, const std::string& val) {
	return {type, val, _line, _col};
}

Token	Lexer::readQuote() {

}

Token	Lexer::readWord() {

}

std::vector<Token> Lexer::tokenize() {
	std::vector<Token> token;
	while (!atEnd()) {
		skipWhitespacesAndComments();
		if (atEnd()) {
			token.push_back(makeToken(TOK_EOF, ""));
			break;
		}
		char c = current();
		if (c == '{') {
			token.push_back(makeToken(TOK_LBRACE, "{"));
			advance();
		} else if (c == '}') {
			token.push_back(makeToken(TOK_RBRACE, "}"));
			advance();
		} else if (c == ';') {
			token.push_back(makeToken(TOK_SEMI, ";"));
			advance();
		} else if (c == '"' || c == '\''){
			token.push_back(readQuote());
		} else {
			token.push_back(readWord());
		}
	return token;
}

