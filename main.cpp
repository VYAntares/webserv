#include "includes/sockets/ServerSocket.hpp"
#include "includes/config/ConfigStruct.hpp"

int	main() {
	Config	test_server;

	ServerSocket server(test_server);

	return 0;
};
