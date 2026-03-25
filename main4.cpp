#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>

static const int PORT = 8080;
static const int BACKLOGS = 64;
static const int MAX_EVENTS = 64;

int create_server_socket() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) { return -1; }

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	return fd;
}

int bind_ports_addr(int server_fd, int PORT) {

}

int	main() {
	int server_fd = create_server_socket();
	if (server_fd == -1)
		{ perror("socket"); return 1; }
	if (bind_ports_addr(server_fd, PORT) == -1)
		{ perror("bind"); return 1; }
	if (listen(server_fd, 64) == -1)
		{ perror("listen"); return 1; }
}
