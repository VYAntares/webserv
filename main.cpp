#include "includes/sockets/ServerSocket.hpp"
#include "includes/config/ConfigStruct.hpp"

int	main() {
	Config	cfg;

	// Server 1 : 0.0.0.0:8080
	Server	s1;
	s1.listen.clear();
	s1.listen.push_back(addrport(INADDR_ANY, htons(8080)));

	// Server 2 : deux adresses differentes (0.0.0.0:8081 + 127.0.0.1:8082)
	Server	s2;
	s2.listen.clear();
	s2.listen.push_back(addrport(INADDR_ANY, htons(8081)));
	s2.listen.push_back(addrport(inet_addr("127.0.0.1"), htons(8082)));
	s2.listen.push_back(addrport(inet_addr("127.0.0.1"), htons(8082)));

	// Server 3 : meme port que s2 sur 0.0.0.0:8081 → doit etre deduplique plus tard
	//            pour l'instant garde un port unique pour tester sans erreur
	Server	s3;
	s3.listen.clear();
	s3.listen.push_back(addrport(inet_addr("127.0.0.1"), htons(9000)));

	cfg.srv.push_back(s1);
	cfg.srv.push_back(s2);
	cfg.srv.push_back(s3);

	ServerSocket server(cfg);

	return 0;
}
