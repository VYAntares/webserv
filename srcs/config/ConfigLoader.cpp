#include "../../includes/config/ConfigLoader.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/config/Token.hpp"
#include "../../includes/config/Lexer.hpp"

#include <sstream>
#include <iomanip>
#include <fstream>

ConfigLoader::ConfigLoader(int argc, char **argv) {
	if (argc != 2)
		throw std::runtime_error("Too many arguments, usage: <configFile path>");

	std::ifstream file(argv[1]);
	if (!file.is_open())
		throw std::runtime_error("Can not open config file: " + std::string(argv[1]));

	std::ostringstream ss;
	ss << file.rdbuf();
	_input = ss.str();

	try {
		startLexer();
		startParser();
		// startValidator();
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "parser: configuration file " << 
			argv[1] << " failed." << std::endl;
	}
}

void ConfigLoader::startLexer() {
	Lexer	l(_input);
	_tokens = l.tokenize();
}

void ConfigLoader::startParser() {
	Parser	p(_tokens);

	p.parse();

}
