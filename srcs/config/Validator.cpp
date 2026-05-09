#include "../../includes/config/Validator.hpp"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <netdb.h>
#include <netinet/in.h>

Validator::Validator(Config c) : _c(c) {}


void Validator::propogateServerToLocations(Server& s) {
	for (size_t i = 0; i < s.locations.size(); i++) {
		Location& l = s.locations[i];
		if (l.root.empty()) {
			std::cout << "ROOT EMPTY, REPLACING WITH :" << s.root << std::endl;
			l.root = s.root;
		}
		if (l.index.empty())
			l.index = s.index;
		if (l.max_body_client == SIZE_MAX)
			l.max_body_client = s.max_body_client;
		if (l.autoindex == -1)
			l.autoindex = s.autoindex;
		if (l.return_path.first == -1) 
			l.return_path = s.return_path;
		for (std::map<int, str>::iterator it = s.error_page.begin();
			it != s.error_page.end(); ++it) {
				if (l.error_page.find(it->first) == l.error_page.end())
					l.error_page[it->first] = it->second;
			}
	}
}



Config Validator::validate() {
	for (size_t i = 0; i < _c.cfg.size(); i++) {
		if (_c.cfg[i].locations.empty())
			throw std::runtime_error("No location block provided to 'Server' : "
									+ (toString(i + 1)));
		propogateServerToLocations(_c.cfg[i]);
	}
	return _c;

	// if (_c.cfg.empty())
	// 	throw std::runtime_error("Config is empty.");

	// for (size_t i = 0; i < _c.cfg.size(); i++) {
	// 	const Server& s = _c.cfg[i];

	// 	if (s.listen.empty())
	// 		throw std::runtime_error();

	// 	if (s.locations.empty())
	// 		throw std::runtime_error();

	// 	for (size_t j = 0; j < s.locations.size(); j++) {
	// 		const Location& l = s.locations[j];
	// 		if (l.root.empty() l.return_path.first == -1)
	// 			throw std::runtime_error();
	// 	}
	// }
}