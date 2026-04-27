#include "../../includes/config/Lexer.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>

Lexer::Lexer(const std::string& input) : _input(input), _pos(0), _line(1), _col(1) {}

// bool si on est ou pas a la fin
bool	Lexer::atEnd() { return _pos >= _input.size(); }

// renvoie le char actuel
char	Lexer::current() { return _input[_pos]; }

// itoa style
static std::string toString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}

// avance et met a jour line et col correctement selon si il y a \n
void	Lexer::advance() {
	if (current() == '\n')	{ _line++; _col = 1; }
	else					{ _col++; }
	_pos++;
}

// le nom de la fonction ne point mieux expliquer son fonctionnement
void	Lexer::skipWhitespacesAndComments() {
	while (!atEnd()) {
		char c = current();
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			advance();
		else if (c == '#') {
			// si c'est un commentaire, lire jusqua la fin ou jusqua \n
			while (!atEnd() && current() != '\n')
				advance();
		} else 
			break;
	}
}

// pareil ici quoi, makeToken en francais: faisToken
Token	Lexer::makeToken(TokenType type, const std::string& value) {
	Token t;
	t.type	= type;
	t.value	= value;
	t.line	= _line;
	t.col	= _col;

	return t;
}

// tokenize ce qu'il y a, a linterieur des quotes
// throw une exception si quote non fermante pas trouvee
Token	Lexer::readQuote(char quote) {
	size_t	startLine	= _line;
	size_t	startCol	= _col;
	std::string	value;
	
	// skip quote ouvrante
	advance();
	// recuperer ce qu'il y a entre jusqu'a quote fermante ou 
	// fin du fichier
	while (!atEnd() && current() != quote) {
		value += current();
		advance();
	}
	// et si fin du fichier, alors quote fermante pas trouvee 
	// donc erreur
	if (atEnd()) {
		throw std::runtime_error(
				"unclosed quote at line: " + toString(startLine) +
				", col: " + toString(startCol)
				);
	}
	// skip quote fermante
	advance();

	Token t;
	t.type	= TOK_WORD;
	t.value	= value;
	t.line	= startLine;
	t.col	= startCol;
	
	return t;
}

// tokenize un word
Token	Lexer::readWord() {
	size_t	startLine	= _line;
	size_t	startCol	= _col;
	std::string value;

	// lire jusqu'a la fin et si on croise un espace, bracket, semi 
	// ou commentaire, alors on forme un mot
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

// tokenizer, prend l'input et nous rend un vector de token.
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
		} else if (c == '"' || c == '\'')
			token.push_back(readQuote(c));
		else 
			token.push_back(readWord());
	}
	return token;
}

