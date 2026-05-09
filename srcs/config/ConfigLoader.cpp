#include "../../includes/config/ConfigLoader.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/config/Token.hpp"
#include "../../includes/config/Validator.hpp"
#include "../../includes/config/Lexer.hpp"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

ConfigLoader::ConfigLoader(int argc, char **argv) {
	if (argc != 2)
		throw std::runtime_error("Too many arguments, usage: <configFile path>");

	size_t colon = std::string(argv[1]).find(".conf");
	if (colon == std::string::npos) 
		throw std::runtime_error("Can not open config file: " + std::string(argv[1])
								+ "\nConfig file must have: .conf extension.");
	else {
		// stat() remplit la struct avec les infos du chemin
		// S_ISREG verifie que c'est un fichier regulier (pas un dossier, socket, etc.)
		struct stat st;
		if (stat(argv[1], &st) != 0 || !S_ISREG(st.st_mode))
			throw std::runtime_error("Not a regular file: " + std::string(argv[1]));

		std::ifstream file(argv[1]);
		if (!file.is_open())
			throw std::runtime_error("Can not open config file: " + std::string(argv[1]));

		std::ostringstream ss;
		ss << file.rdbuf();
		_input = ss.str();
		if (_input.empty())
			throw std::runtime_error("Config file: '" + std::string(argv[1]) 
									+ "' is empty. No server will run like that..");

		try {
			startLexer();
			startParser();
			startValidator();
		} catch (std::exception &e) {
			throw std::runtime_error(std::string(e.what()) + "\nparser: configuration file "
									+ std::string(argv[1]) + " failed.");
		}
	}
}

void ConfigLoader::startLexer() {
	Lexer	l(_input);
	_tokens = l.tokenize();
}

void ConfigLoader::startParser() {
	Parser	p(_tokens);
	_c = p.parse();
}

void ConfigLoader::startValidator() {
	Validator v(_c);

	_c = v.validate();
}

Config	ConfigLoader::getConfig() { return _c; }

