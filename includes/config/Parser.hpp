#pragma once

#include "ConfigStruct.hpp"
#include "Token.hpp"

#include <fstream>

class Parser {
	public:
		Parser(int argc, char** argv);
		void parse();

	private:
		std::vector<Token>			_tokens;
		size_t						_pos;
};

