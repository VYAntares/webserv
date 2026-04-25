#include "includes/handlers/ServerHandler.hpp"
#include "includes/core/EventLoop.hpp"
#include <iostream>

int main() {
		// ConfigParser parser(argc, argv);
		// parser.parse();


		Config	c;

		// Server 1 : 0.0.0.0:8080
		Server	s1;
		s1.listen.clear();
		s1.listen.push_back(addrport(INADDR_ANY, 8080));

		// Server 2 : deux adresses differentes (0.0.0.0:8081 + 127.0.0.1:8082)
		Server	s2;
		s2.listen.clear();
		s2.listen.push_back(addrport(INADDR_ANY, 8081));
		s2.listen.push_back(addrport(inet_addr("127.0.0.1"), 8082));
		s2.listen.push_back(addrport(inet_addr("127.0.0.1"), 8082));

		// Server 3 : meme port que s2 sur 0.0.0.0:8081 → doit etre deduplique plus tard
		//            pour l'instant garde un port unique pour tester sans erreur
		Server	s3;
		s3.listen.clear();
		s3.listen.push_back(addrport(inet_addr("127.0.0.1"), 9000));

		c.cfg.push_back(s1);
		c.cfg.push_back(s2);
		c.cfg.push_back(s3);

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

	return 0;
}

