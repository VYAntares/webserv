#pragma once

#include "ConfigStruct.hpp"

#include <set>
#include <fstream>

class ConfigParser {
	public:
		ConfigParser(const std::string& filepath);
		void parse();
		const std::vector<Server>& getServers() const;

	private:
		std::vector<std::string>	_tokens;
		std::vector<Server>			_servers;
		std::ifstream				_file;
		std::string					_filepath;
		std::size_t					_pos;
};

