#include "../../includes/config/Parser.hpp"
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/config/Token.hpp"
#include "../../includes/config/Lexer.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>

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

Parser::Parser(int argc, char** argv) : _pos(0) {
	if (argc != 2) 
		throw std::runtime_error("Too many arguments, usage: <configFile path>");

	std::ifstream file(argv[1]);
	if (!file.is_open())
		throw std::runtime_error("Cannot open config file: " + std::string(argv[1]));

	std::ostringstream ss;
	ss << file.rdbuf();
	std::string input = ss.str();
	
	Lexer lex(input);
	_tokens = lex.tokenize();

	for (size_t i = 0; i < _tokens.size(); i++) {
		std::cout << "Type: " << std::setw(10) << std::left << tokenTypeToString(_tokens[i].type)
				  << " Value: " << _tokens[i].value << std::endl;
	}
}

void	Parser::parse() {
}

