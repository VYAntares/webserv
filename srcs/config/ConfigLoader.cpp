#include "../../includes/config/ConfigLoader.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/config/Validator.hpp"
#include "../../includes/config/Lexer.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/stat.h>



static const std::string DEFAULT_CONFIG_PATH = "./conf/default.conf";



ConfigLoader::ConfigLoader(int argc, char **argv) {
	if (argc > 2)
		throw std::runtime_error("Too many arguments, usage: ./webserv [configFile path]");

	std::string path = (argc == 2) ? std::string(argv[1]) : DEFAULT_CONFIG_PATH;

	if (argc == 1)
		std::cerr << "No config file provided, using default: " << DEFAULT_CONFIG_PATH << std::endl;

	if (path.find(".conf") == std::string::npos)
		throw std::runtime_error("Can not open config file: " + path
								+ "\nConfig file must have: .conf extension.");

	struct stat st;
	if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
		throw std::runtime_error("Not a regular file: " + path);

	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("Can not open config file: " + path);

	std::ostringstream ss;
	ss << file.rdbuf();
	_input = ss.str();
	if (_input.empty())
		throw std::runtime_error("Config file: '" + path + "' is empty. No server will run like that..");

	try {
		startLexer();
		startParser();
		startValidator();
	} catch (std::exception &e) {
		throw std::runtime_error(std::string(e.what()) + "\nparser: configuration file "
								+ path + " failed.");
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

