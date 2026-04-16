#pragma once

#include <string>
#include <vector>
#include <utility>

typedef std::string             str;
typedef std::pair<int, str>     intstr;				// code, string
typedef std::pair<str, str>     strstr;				// string, string
typedef std::pair<str, int>     strint;				// string, port

struct Location {
	// Propre à Location
	std::vector<strstr>			cgi_pass;			// extension -> chemin interpreteur (.py -> /usr/bin/python)
	std::vector<str>			allowed_methods;	// methodes HTTP autorisees (GET, POST, DELETE)
	std::string					upload_store;		// dossier ou stocker les fichiers uploades
	intstr						return_path;		// code redirect + URL cible (-1 = pas de redirect)
	std::string					path;				// URL/route du bloc (/api, /upload, /)

	// Herite du Server si vide
	std::vector<intstr>			error_page;			// code erreur -> page HTML (404 -> /errors/404.html)
	std::string					root;				// dossier racine des fichiers servis
	std::string					index;				// fichier par defaut (index.html)
	int							autoindex;			// listing du dossier (-1 = non defini, 0 = off, 1 = on)

	Location() : return_path(-1, ""), autoindex(-1) {}
};

struct Server {
	// Propre au Server
	std::vector<strint>			listen;				// adresse + port d'ecoute (0.0.0.0:8080)
	std::string					server_name;		// nom de domaine (localhost, example.com)
	size_t						max_body_client;	// taille max du body en octets (defaut: 1MB)

	// Location en herite si vide
	std::vector<intstr>			error_page;			// code erreur -> page HTML (404 -> /errors/404.html)
	std::string					root;				// dossier racine des fichiers servis
	std::string					index;				// fichier par defaut (index.html)
	int							autoindex;			// listing du dossier (-1 = non defini, 0 = off, 1 = on)

	std::vector<Location>		locations;			// blocs location du server

	Server() : max_body_client(1024 * 1024), autoindex(-1) {}
};

struct Config {
	std::vector<Server>			servers;			// tous les serveurs du fichier de config
};
