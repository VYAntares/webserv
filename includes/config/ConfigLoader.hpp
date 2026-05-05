#pragma once

#include "Token.hpp"
#include "ConfigStruct.hpp"

#include <iostream>
#include <vector>

class ConfigLoader {
	public:
		ConfigLoader(int argc, char **argv);

		Config				getConfig();
		void				printConfig() const;

	private:
		Config				_c;

		std::string			_input;
		std::vector<Token>	_tokens;

		void				startLexer();
		void				startParser();
		void				startValidator();
};

