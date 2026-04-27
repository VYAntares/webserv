#include "../../includes/config/Parser.hpp"
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/config/Token.hpp"
#include "../../includes/config/Lexer.hpp"

#include <iostream>
#include <sstream>

Parser::Parser(int argc, char** argv) : _pos(0) {
	if (argc != 2) 
		throw std::runtime_error("Too many arguments, usage: <configFile path>");

	std::ifstream file(argv[1]);
	if (!file.is_open())
		throw std::runtime_error("Cannot open config file: " + std::string(argv[1]));

	std::ostringstream ss;
	ss << file.rdbuf();
	std::string input = ss.str();

	// Lexer lexer(input);
	// _tokens = lexer.tokenize();
	
	std::cout << input << std::endl;
}

void	Parser::parse() {
}

