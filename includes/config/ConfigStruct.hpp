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

typedef std::string             	str;
typedef std::pair<uint32_t, int>	addrport;			// string, port

struct BaseBlock {
	std::pair<int, str>				return_path;		// code redirect + URL cible (-1 = pas de redirect)
	std::map<int, str>				error_page;			// code erreur -> page HTML (404 -> /errors/404.html)
	std::string						root;				// dossier racine des fichiers servis
	std::string						index;				// fichier par defaut (index.html)
	size_t							max_body_client;	// taille max du body en octets (defaut: 1MB)
	int								autoindex;			// listing du dossier (-1 = non defini, 0 = off, 1 = on)

	BaseBlock() : return_path(-1, ""), max_body_client(SIZE_MAX), autoindex(-1) {}
};

struct Location : public BaseBlock {
	// Propre à Location
	std::map<str, str>				cgi_pass;			// extension -> chemin interpreteur (.py -> /usr/bin/python)
	std::vector<str>				allowed_methods;	// methodes HTTP autorisees (GET, POST, DELETE)
	std::string						upload_store;		// dossier ou stocker les fichiers uploades
	std::string						path;				// URL/route du bloc (/api, /upload, /)

	Location() {}
};

struct Server : public BaseBlock {
	// Propre au Server
	std::vector<addrport>			listen;				// adresse + port d'ecoute (0.0.0.0:8080)
	std::vector<str>				server_name;		// nom de domaine (localhost, example.com)

	std::vector<Location>			locations;			// blocs location du server

	Server() {}
};

struct Config {
	std::vector<Server>				cfg;				// tous les serveurs du fichier de config
};

