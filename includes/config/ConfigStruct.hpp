#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <sstream>
#include <arpa/inet.h>
#include <stdint.h>

// inline : ce header est inclus par plusieurs fichiers .cpp (Parser.cpp,
// ServerHandler.cpp...). Une fonction normale définie dans un header
// serait donc dupliquée dans chaque .cpp qui l'inclut, et l'édition de
// liens échouerait (même symbole défini plusieurs fois). inline dit au
// linker "ce n'est pas grave si tu la vois plusieurs fois, garde-en une
// seule" — c'est ce qui permet de définir directement la fonction ici au
// lieu de la déclarer dans le .hpp et de l'implémenter dans un .cpp à part.
inline std::string toString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}



// addrToStr convertit une adresse IPv4 en notation "a.b.c.d" (ex: "127.0.0.1").
// L'adresse arrive stockée dans un seul uint32_t (sin_addr.s_addr, voir
// Parser::resolveHost) : les 4 octets de l'adresse sont collés bout à bout
// dans ces 4 octets. Pour les ré-extraire un par un :
//   - "& 0xFF" ne garde que les 8 derniers bits (un octet), le reste passe à 0
//   - ">> 8", ">> 16", ">> 24" décalent d'abord l'entier de 1, 2 puis 3
//     octets vers la droite, pour amener l'octet suivant à la fin avant de
//     le masquer avec & 0xFF
// Comme sin_addr.s_addr est déjà rangé dans l'ordre réseau (le premier
// octet de l'adresse, ex: 127, est le premier en mémoire), ce décalage
// simple suffit à les ressortir dans le bon ordre sans passer par ntohl().
//
// inline pour la même raison que toString : header partagé par plusieurs
// .cpp, donc pas de vraie définition unique possible sans ce mot-clé.
inline std::string	addrToStr(uint32_t addr) {
	std::ostringstream oss;
	oss << (addr & 0xFF) << "."
		<< ((addr >> 8) & 0xFF) << "."
		<< ((addr >> 16) & 0xFF) << "."
		<< ((addr >> 24) & 0xFF);
	return oss.str();
}



// ecrire ses truc etait trop long. donc define lol.
typedef std::string             	str;
typedef std::pair<uint32_t, int>	addrport;			// string, port



// BaseBlock possede les directive commune de server et location.
struct BaseBlock {
	std::pair<int, str>				return_path;		// code redirect + URL cible (-1 = pas de redirect)
	std::map<int, str>				error_page;			// code erreur -> page HTML (404 -> /errors/404.html)
	std::string						root;				// dossier racine des fichiers servis
	std::string						index;				// fichier par defaut (index.html)
	size_t							max_body_client;	// taille max du body en octets (defaut: 1MB)
	int								autoindex;			// listing du dossier (-1 = non defini, 0 = off, 1 = on)

	BaseBlock() : return_path(-1, ""), max_body_client(SIZE_MAX), autoindex(-1) {}
};



// location herite de BaseBlock (directive commune avec server)
struct Location : public BaseBlock {
	// Propre à Location
	std::map<str, str>				cgi_pass;			// extension -> chemin interpreteur (.py -> /usr/bin/python)
	std::vector<str>				allowed_methods;	// methodes HTTP autorisees (GET, POST, DELETE)
	std::string						upload_store;		// dossier ou stocker les fichiers uploades
	std::string						path;				// URL/route du bloc (/api, /upload, /)

	Location() {}
};



// serveur herite de BaseBlock (directive commune a location et server)
// et il a plusieur location aussi.
struct Server : public BaseBlock {
	// Propre au Server
	std::vector<addrport>			listen;				// adresse + port d'ecoute (0.0.0.0:8080)
	std::vector<str>				server_name;		// nom de domaine (localhost, example.com)

	std::vector<Location>			locations;			// blocs location du server

	Server() {}
};



// config possede plusieurs configuration serveur.
struct Config {
	std::vector<Server>				cfg;				// tous les serveurs du fichier de config
};

