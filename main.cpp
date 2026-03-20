#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

int create_socket_server() {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		std::cerr << "socket() failed" << std::endl;
		return -1;
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	std::cout << "socket() ok - fd = " << server_fd << std::endl;
	return server_fd;
}



int bind_ip_and_ports_to_server(int server_fd) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		std::cerr << "bind() failed" << std::endl;
		return -1;
	}
	
	std::cout << "bind() ok - port 8080" << std::endl;
	return 0;
}



int accept_client(int server_fd) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd == -1) {
		std::cerr << "accept() failed" << std::endl;
		return -1;
	}

	std::cout << "accept() ok - client connecte" << std::endl;
	return client_fd;
}



void receive_and_send(int client_fd) {
	char buf[4096];

	int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
	buf[n] = '\0';
	std::cout << "--- REQUETE HTTP ---" << std::endl;
	std::cout << buf << std::endl;

	const char *response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text\r\n"
		"Content-Length: 21\r\n"
		"\r\n"
		"<h1>Hello World!</h1>";
	send(client_fd, response, strlen(response), 0);
	close(client_fd);
}



int main() {
	int server_fd = create_socket_server();
	if (server_fd == -1) return 1;

	if (bind_ip_and_ports_to_server(server_fd) == -1) return 1;

	if (listen(server_fd, 10) == -1) {
		std::cerr << "listen() failed" << std::endl;
		return 1;
	}
	std::cout << "listen() ok" << std::endl;

	while (1) {
		int client_fd = accept_client(server_fd);

		if (client_fd == -1) continue;

		receive_and_send(client_fd);
	}
	return 0;
}

