#include "includes/handlers/ServerHandler.hpp"
#include "includes/config/Parser.hpp"
#include "includes/core/EventLoop.hpp"
#include "includes/config/ConfigLoader.hpp"
#include <iostream>

// bienvenue dans la porte d'entree nous menant au paradis.
// ici on lance l'objet qui va load la config et la parser.
// une fois la config obtenue, on lance un serveur pour
// chaque config server du fichier de conf, et surtout
// chaque listen represente un serveur qui ecoute sur
// une paire adresse et port.
// et on lance la boucle evenementielle en appelant la seule
// instance de EventLoop.
int main(int argc, char **argv) {
	try {
		ConfigLoader cl(argc, argv);
		Config c = cl.getConfig();
		for (size_t i = 0; i < c.cfg.size(); i++) {
			for (size_t j = 0; j < c.cfg[i].listen.size(); j++) {
				try {
					new ServerHandler(c.cfg[i].listen[j], c.cfg[i]);
				} catch (std::exception &e) {
					std::cerr << e.what() << std::endl;
				}
			}
		}
		EventLoop::instance()->handle_events();
		EventLoop::destroy();
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}

