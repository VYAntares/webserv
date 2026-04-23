#pragma once

#include "ConfigStruct.hpp"

#include <fstream>

class ConfigParser {
	public:
		ConfigParser(int argc, char** argv);
		void parse();

	private:
		std::vector<std::string>	_tokens;
		std::ifstream				_file;
		size_t						_pos;
};

