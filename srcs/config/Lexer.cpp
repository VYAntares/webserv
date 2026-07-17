#include "../../includes/config/Lexer.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

Lexer::Lexer(const std::string& input) : _input(input), _pos(0), _line(1), _col(1) {}

bool	Lexer::atEnd() { return _pos >= _input.size(); }

char	Lexer::current() { return _input[_pos]; }

static std::string toString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}

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

Token	Lexer::makeToken(TokenType type, const std::string& value) {
	Token t;
	t.type	= type;
	t.value	= value;
	t.line	= _line;
	t.col	= _col;

	return t;
}

Token	Lexer::readQuote(char quote) {
	size_t	startLine	= _line;
	size_t	startCol	= _col;
	std::string	value;

	advance();
	while (!atEnd() && current() != quote) {
		value += current();
		advance();
	}
	if (atEnd()) {
		throw std::runtime_error(
				"Tokenizer: unclosed quote at line: " + toString(startLine) +
				", col: " + toString(startCol)
				);
	}
	advance();

	Token t;
	t.type	= TOK_WORD;
	t.value	= value;
	t.line	= startLine;
	t.col	= startCol;
	
	return t;
}

Token	Lexer::readWord() {
	size_t	startLine	= _line;
	size_t	startCol	= _col;
	std::string value;

	while (!atEnd()) {
		char c = current();
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n'
			|| c == '{' || c == '}' || c == ';' || c == '#')
			break;
		value += c;
		advance();
	}

	Token t;
	t.type	= TOK_WORD;
	t.value	= value;
	t.line	= startLine;
	t.col	= startCol;

	return t;
}

static std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TOK_WORD:   return "WORD";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_SEMI:   return "SEMI";
        case TOK_EOF:    return "EOF";
        default:         return "UNKNOWN";
    }
}

std::vector<Token> Lexer::tokenize() {
	std::vector<Token> token;
	while (!atEnd()) {
		skipWhitespacesAndComments();
		if (atEnd())
			break;
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
		} else if (c == '"' || c == '\'')
			token.push_back(readQuote(c));
		else
			token.push_back(readWord());
	}
	token.push_back(makeToken(TOK_EOF, ""));
	for (size_t i = 0; i < token.size(); i++) {
		std::cout << "Type: " << std::setw(10) << std::left << tokenTypeToString(token[i].type)
				  << " Value: " << std::setw(30) << std::left << token[i].value
				  << " Col: " << std::setw(3) << std::left << token[i].col << " Line: " << token[i].line << std::endl;
	}
	return token;
}

