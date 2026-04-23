#include "../../includes/config/ConfigParser.hpp"
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/config/Token.hpp"

#include <iostream>

ConfigParser::ConfigParser(int argc, char** argv) : _pos(0) {
	if (argc != 2) 
		throw std::runtime_error("Too many arguments, usage: <configFile path>");
	_file.open(argv[1]);
	if (!_file.is_open())
		throw std::runtime_error("Cannot open config file: " + std::string(argv[1]));
}

// void	ConfigParser::parse() {
// 	char		c;
// 	std::string	word;

// 	while (_file.get(c)) {
// 		if () {
			
// 		} else 
// 			word += c;
// 	}
// }

