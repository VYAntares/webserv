#pragma once

#include "ConfigStruct.hpp"
#include "Token.hpp"

class Parser {
	public:
		Parser(const std::vector<Token>& tokens);
		void parse();

	private:
		size_t				_pos;
		std::vector<Token>	_tokens;
		Config				_c;

		void parseServerBlock();
		void parseLocationBlock(Server& s);

		template<typename T>
		void parseDirective(T& block);

		Token	current();
		Token	consume();
		Token	except(TokenType type);
};
