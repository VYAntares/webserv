#include "../../includes/config/Lexer.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

Lexer::Lexer(const std::string& input) : _input(input), _pos(0), _line(1), _col(1) {}

// retourne vrai si on est à la fin
bool	Lexer::atEnd() { return _pos >= _input.size(); }

// renvoie le char actuel
char	Lexer::current() { return _input[_pos]; }

// entier vers string
static std::string toString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}

// avance et met à jour line et col selon la présence d'un \n
void	Lexer::advance() {
	if (current() == '\n')	{ _line++; _col = 1; }
	else					{ _col++; }
	_pos++;
}

// le nom de la fonction ne pourrait mieux expliquer son fonctionnement
void	Lexer::skipWhitespacesAndComments() {
	while (!atEnd()) {
		char c = current();
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			advance();
		else if (c == '#') {
			// si c'est un commentaire, lire jusqu'à la fin de ligne ou jusqu'à \n
			while (!atEnd() && current() != '\n')
				advance();
		} else
			break;
	}
}

// pareil ici, makeToken en français : créeToken
Token	Lexer::makeToken(TokenType type, const std::string& value) {
	Token t;
	t.type	= type;
	t.value	= value;
	t.line	= _line;
	t.col	= _col;

	return t;
}

// tokenize ce qu'il y a à l'intérieur des quotes
// throw une exception si la quote fermante n'est pas trouvée
Token	Lexer::readQuote(char quote) {
	size_t	startLine	= _line;
	size_t	startCol	= _col;
	std::string	value;

	// saute la quote ouvrante
	advance();
	// récupère ce qu'il y a jusqu'à la quote fermante ou
	// la fin du fichier
	while (!atEnd() && current() != quote) {
		value += current();
		advance();
	}
	// si fin du fichier, alors quote fermante pas trouvée
	// donc erreur
	if (atEnd()) {
		throw std::runtime_error(
				"Tokenizer: unclosed quote at line: " + toString(startLine) +
				", col: " + toString(startCol)
				);
	}
	// saute la quote fermante
	advance();

	Token t;
	t.type	= TOK_WORD;
	t.value	= value;
	t.line	= startLine;
	t.col	= startCol;
	
	return t;
}

// tokenize un mot
Token	Lexer::readWord() {
	size_t	startLine	= _line;
	size_t	startCol	= _col;
	std::string value;

	// lit jusqu'à la fin, et si on croise un espace, bracket, semi
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

// prend l'input et retourne un vecteur de tokens.
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
	// debug list all Token struct values
	for (size_t i = 0; i < token.size(); i++) {
		std::cout << "Type: " << std::setw(10) << std::left << tokenTypeToString(token[i].type)
				  << " Value: " << std::setw(30) << std::left << token[i].value
				  << " Col: " << std::setw(3) << std::left << token[i].col << " Line: " << token[i].line << std::endl;
	}
	return token;
}

