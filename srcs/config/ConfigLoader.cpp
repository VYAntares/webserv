#include "../../includes/config/ConfigLoader.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/config/ConfigStruct.hpp"
#include "../../includes/config/Validator.hpp"
#include "../../includes/config/Lexer.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/stat.h>



static const std::string DEFAULT_CONFIG_PATH = "./conf/default.conf";


// ConfigLoader vérifie le bon lancement du programme : le nombre
// d'arguments, que le fichier de config existe et n'est pas vide,
// puis lance le parsing (lexer → parser → validator).
ConfigLoader::ConfigLoader(int argc, char **argv) {
	// maximum 2 arguments pour lancer le programme (le binaire + un path optionnel).
	if (argc > 2)
		throw std::runtime_error("Too many arguments, usage: ./webserv [configFile path]");

	// s'il y a deux arguments, un path est fourni ; sinon on prend le path par défaut
	std::string path = (argc == 2) ? std::string(argv[1]) : DEFAULT_CONFIG_PATH;
	if (argc == 1)
		std::cerr << "No config file provided, using default: " << DEFAULT_CONFIG_PATH << std::endl;

	// vérifier que le fichier finit bien en .conf
	if (path.find(".conf") == std::string::npos)
		throw std::runtime_error("Can not open config file: " + path
								+ "\nConfig file must have: .conf extension.");

	// stat() va demander au système de fichiers les infos sur ce path
	// (type de fichier, taille, permissions, dates...) sans avoir besoin
	// de l'ouvrir. Elle les range dans st, et S_ISREG lit dans st.st_mode
	// le bit qui dit "c'est un fichier régulier" — ça exclut un dossier,
	// un lien symbolique, une socket, etc. Sans ce test, on pourrait nous
	// donner un dossier nommé "truc.conf" et laisser le reste du code se
	// comporter n'importe comment avec.
	struct stat st;
	if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
		throw std::runtime_error("Not a regular file: " + path);

	// ifstream essaie d'ouvrir le fichier en lecture ; is_open() dit si ça
	// a marché. Le stat() au-dessus ne suffit pas : il ne vérifie pas les
	// droits de lecture, et rien n'empêche le fichier de disparaître entre
	// les deux appels — is_open() est le seul vrai juge de "j'ai réussi à
	// l'ouvrir ou pas".
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("Can not open config file: " + path);

	// rdbuf() donne le buffer interne du fichier ; le faire couler dans ss
	// avec << recopie tout le contenu d'un coup dans le stringstream,
	// plus simple que de lire ligne par ligne. Si le fichier est vide,
	// _input reste vide et on refuse de démarrer.
	std::ostringstream ss;
	ss << file.rdbuf();
	_input = ss.str();
	if (_input.empty())
		throw std::runtime_error("Config file: '" + path + "' is empty. No server will run like that..");

	// lancement du processus de parsing
	try {
		startLexer();
		startParser();
		startValidator();
	} catch (std::exception &e) {
		throw std::runtime_error(std::string(e.what()) + "\nparser: configuration file "
								+ path + " failed.");
	}
}


// lancement du lexer et récupération des tokens
void ConfigLoader::startLexer() {
	Lexer	l(_input);
	_tokens = l.tokenize();
}


// lancement du parser et récupération de la config
void ConfigLoader::startParser() {
	Parser	p(_tokens);
	_c = p.parse();
}


// validation et mise à jour de la config
void ConfigLoader::startValidator() {
	Validator v(_c);

	_c = v.validate();
}



Config	ConfigLoader::getConfig() { return _c; }

