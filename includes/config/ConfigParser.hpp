#pragma once

#include "ConfigStruct.hpp"

#include <fstream>

class ConfigParser {
	public:
		ConfigParser(const std::string& filepath);
		void parse();

	private:
		std::vector<std::string>	_tokens;
		std::ifstream				_file;
		size_t						_pos;
};

