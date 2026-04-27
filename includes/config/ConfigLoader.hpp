#pragma once

#include "Token.hpp"

#include <iostream>
#include <vector>

class ConfigLoader {
	public:
		ConfigLoader(int argc, char **argv);
	
	private:
		std::string			_input;
		std::vector<Token>	_tokens;

		void startLexer();
		void startParser();
		void startValidator();
};

