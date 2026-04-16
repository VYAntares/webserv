#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>

struct LocationConfig {
	std::string							prefix;
	std::string							root;
	std::string							index;
	bool								autoindex;
	std::set<std::string>				allowed_methods;
	bool								has_redirect;
	int									redirect_code;
	std::string							redirect_url;
	std::string							upload_store;
	std::map<std::string, std::string>	cgi_handlers;

};

struct ServerConfig {
	std::string					host;
	int							port;
	std::string					server_name;
	std::string					root;
	std::string					index;
	size_t						client_max_body;
	std::map<int, std::string>	error_pages;
	std::vector<LocationConfig>	locations;
};

class Config {
public:
	Config(const std::string& filepath);
	void parse();
	const std::vector<ServerConfig>& getServers() const;

private:
	std::vector<ServerConfig>	_servers;
	std::string					_filepath;
	std::ifstream				_file;
	std::vector<std::string>	_tokens;
	std::size_t					_pos;
	
};

}
