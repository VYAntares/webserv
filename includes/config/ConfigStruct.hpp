#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <arpa/inet.h>

typedef std::string             	str;
typedef std::pair<uint32_t, int>	addrport;		// string, port

struct Location {
	// Propre à Location
	std::pair<int, str>			return_path;		// code redirect + URL cible (-1 = pas de redirect)
	std::map<str, str>			cgi_pass;			// extension -> chemin interpreteur (.py -> /usr/bin/python)
	std::vector<str>			allowed_methods;	// methodes HTTP autorisees (GET, POST, DELETE)
	std::string					upload_store;		// dossier ou stocker les fichiers uploades
	std::string					path;				// URL/route du bloc (/api, /upload, /)

	// Herite du Server si vide
	std::map<int, str>			error_page;			// code erreur -> page HTML (404 -> /errors/404.html)
	std::string					root;				// dossier racine des fichiers servis
	std::string					index;				// fichier par defaut (index.html)
	size_t						max_body_client;	// taille max du body en octets (defaut: 1MB)
	int							autoindex;			// listing du dossier (-1 = non defini, 0 = off, 1 = on)

	Location() : return_path(-1, ""), autoindex(-1) {}
};

// Directive            Validation          -ne figure pas dans nginx
//
// allowed_methods      valeurs parmi GET/POST/DELETE, au moins une
// cgi_pass             2 args, extension commence par ., chemin valide
// upload_store         1 arg, chemin absolu, dossier accessible en écriture

struct Server {
	// Propre au Server
	std::vector<addrport>		listen;				// adresse + port d'ecoute (0.0.0.0:8080)
	std::string					server_name;		// nom de domaine (localhost, example.com)

	// Location en herite si vide
	std::map<int, str>			error_page;			// code erreur -> page HTML (404 -> /errors/404.html)
	std::string					root;				// dossier racine des fichiers servis
	std::string					index;				// fichier par defaut (index.html)
	size_t						max_body_client;	// taille max du body en octets (defaut: 1MB)
	int							autoindex;			// listing du dossier (-1 = non defini, 0 = off, 1 = on)

	std::vector<Location>		locations;			// blocs location du server

	// Server() : max_body_client(1024 * 1024), autoindex(-1) {}
	
	Server() : max_body_client(1024 * 1024), autoindex(-1) {
    listen.push_back(addrport(INADDR_ANY, 8080));
	listen.push_back(addrport(inet_addr("127.0.0.1"), 8081));
    server_name = "localhost";
    root        = "/var/www/main";
    index       = "index.html";
    error_page[404] = "/errors/404.html";
    error_page[500] = "/errors/500.html";
	}
};

struct Config {
	std::vector<Server>			srv;			// tous les serveurs du fichier de config
};

