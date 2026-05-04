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

		void 		parseServerBlock();
		void		parseLocationBlock(Server& s);

		void 		parseDirective(Server& s);
		void 		parseDirective(Location& l);
		void 		putDirective(BaseBlock& b, const std::string& key);

		void						parseErrorPage(BaseBlock& b);
		int							parseAutoindex();
		std::pair<int, std::string>	parseReturn();
		size_t						parseClientBody();
		addrport					parseListen();

		Token		current();
		Token		consume();
		Token		expect(TokenType type);
};
