#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <sstream>
#include <arpa/inet.h>
#include <stdint.h>

inline std::string toString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}

inline std::string	addrToStr(uint32_t addr) {
	std::ostringstream oss;
	oss << (addr & 0xFF) << "."
		<< ((addr >> 8) & 0xFF) << "."
		<< ((addr >> 16) & 0xFF) << "."
		<< ((addr >> 24) & 0xFF);
	return oss.str();
}

typedef std::string             	str;
typedef std::pair<uint32_t, int>	addrport;

struct BaseBlock {
	std::pair<int, str>				return_path;
	std::map<int, str>				error_page;
	std::string						root;
	std::string						index;
	size_t							max_body_client;
	int								autoindex;

	BaseBlock() : return_path(-1, ""), max_body_client(SIZE_MAX), autoindex(-1) {}
};

struct Location : public BaseBlock {
	std::map<str, str>				cgi_pass;
	std::vector<str>				allowed_methods;
	std::string						upload_store;
	std::string						path;

	Location() {}
};

struct Server : public BaseBlock {
	std::vector<addrport>			listen;
	std::vector<str>				server_name;

	std::vector<Location>			locations;

	Server() {}
};

struct Config {
	std::vector<Server>				cfg;
};

