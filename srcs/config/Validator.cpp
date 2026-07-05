#include "../../includes/config/Validator.hpp"

#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <netdb.h>
#include <netinet/in.h>

Validator::Validator(Config c) : _c(c) {}


void Validator::propogateServerToLocations(Server& s) {
	for (size_t i = 0; i < s.locations.size(); i++) {
		Location& l = s.locations[i];
		if (l.root.empty())
			l.root = s.root;
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


void Validator::checkLocationConfig(Location& l) {
	// check au moins root ou return path
	if (l.root.empty() && l.return_path.first == -1)
        throw std::runtime_error("Location '" + l.path 
								+ "' has no 'root' or 'return'");
	
	// check allowed methods
	const std::string valid[] = {"GET", "POST", "DELETE"};
	for (size_t i = 0; i < l.allowed_methods.size(); i++) {
		bool found = false;
		for (size_t j = 0; j < 3; j++)
			if (l.allowed_methods[i] == valid[j])
				found = true;
		if (!found)
			throw std::runtime_error("Unknown method '" + l.allowed_methods[i] 
									+ "' in location '" + l.path + "'");
		for (size_t j = i + 1; j < l.allowed_methods.size(); j++)
			if (l.allowed_methods[i] == l.allowed_methods[j])
				throw std::runtime_error("Duplicate method '" + l.allowed_methods[i]
										+ "' in location '" + l.path + "'");
	}

	// check si le error_page est bien une string
	for (std::map<int, std::string>::iterator it = l.error_page.begin();
												it != l.error_page.end(); ++it) {
		if (it->second[0] != '/')
    		throw std::runtime_error("error_page path '" + it->second
									+ "' must be an absolute path in location '"
									+ l.path + "'");
	}

	// si upload_store est fourni, methode POST doit l'etre aussi
	if (!l.upload_store.empty()) {
		bool found = false;
		for (size_t i = 0; i < l.allowed_methods.size(); i++) {
			if (l.allowed_methods[i] == "POST")
				found = true;
		}
		if (!found)
			throw std::runtime_error("Location '" + l.path
									+ "': 'upload_store' requires 'POST' in allowed_methods");
	}

	// verifier que lextension cgi_pass commence par un '.'
	// et que le chemin commence par '/'
	for (std::map<str, str>::iterator it = l.cgi_pass.begin();
										it != l.cgi_pass.end(); ++it) {
		if (it->first[0] != '.')
			throw std::runtime_error("Location '" + l.path + "': cgi_pass extension '"
									+ it->first + "' must start with '.'");
		if (it->second[0] != '/')
			throw std::runtime_error("Location '" + l.path + "': cgi_pass interpreter '"
									+ it->second + "' must be an absolute path");
	}
}


void	Validator::checkDuplicateIntraServer(Server& s, size_t& i) {
	for (size_t k = i + 1; k < _c.cfg.size(); k++) {
				Server& s2 = _c.cfg[k];
				for (size_t l = 0; l < s.listen.size(); l++) {
					for (size_t p = 0; p < s2.listen.size(); p++) {
						if (s.listen[l] == s2.listen[p])
							throw std::runtime_error("duplicate listen '"
													+ addrToStr(s.listen[l].first) + ":"
													+ toString(s.listen[l].second)
													+ "' between server " + toString(i + 1)
													+ " and server " + toString(k + 1));
			}
		}
	}
}


Config Validator::validate() {
	if (_c.cfg.empty())
		throw std::runtime_error("Config is empty.");

	for (size_t i = 0; i < _c.cfg.size(); i++) {
		Server& s = _c.cfg[i];
		if (s.listen.empty())
			throw std::runtime_error("No 'listen' directive in server " + toString(i + 1));
		if (s.locations.empty())
			throw std::runtime_error("No location block provided to 'Server' : "
									+ (toString(i + 1)));
		checkDuplicateIntraServer(s, i);
		propogateServerToLocations(s);
		for (size_t j = 0; j < s.locations.size(); j++)
			checkLocationConfig(s.locations[j]);
	}
	return _c;
}
