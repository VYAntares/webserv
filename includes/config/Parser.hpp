#pragma once

#include "ConfigStruct.hpp"
#include "Token.hpp"

class Parser {
	public:
		Parser(const std::vector<Token>& tokens);

		Config				parse();

	private:
		size_t				_pos;
		std::vector<Token>	_tokens;
		Config				_c;

		void 				parseServerBlock();
		void				parseLocationBlock(Server& s);

		void 				parseDirective(Server& s);
		void 				parseDirective(Location& l);
		void 				putDirective(BaseBlock& b, const std::string& key);

		void				parseErrorPage(BaseBlock& b);
		int					parseAutoindex();
		size_t				parseClientBody();
		addrport			parseListen(Server& s);
		void				splitHostAndPort(std::string& host, std::string& port, std::string& val);
		uint32_t			resolveHost(std::string& host, std::string& port);
		std::pair<int, str>	parseReturn();

		Token				current();
		Token				consume();
		Token				expect(TokenType type);
};
