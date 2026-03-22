#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <sys/epoll.h>

static const int PORT = 8080;
static const int BACKLOGS = 10;
static const int MAX_EVENTS = 64;

int	create_socket_server() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) { perror("socket"); return -1; }

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	return fd;
}

int bind_ports(int server_fd, int port) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr	= INADDR_ANY;
	addr.sin_port			= htons(port);
	addr.sin_family			= AF_INET;

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		{ perror("bind"); return -1; }
	return 0;
}

void accept_next_client(int server_fd, int epfd) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(server_fd,
                            (struct sockaddr*)&client_addr, &len);
	if (client_fd == -1) { perror("accept"); return; }

	fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
	
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = client_fd;
	epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, &ev);
}

void handle_recv(int fd, int epfd) {

}

void handle_send(int fd, int epfd) {

}

int	main() {
	signal(SIGPIPE, SIG_IGN);

	// 1 Etape: socket(), bind(), listen()
	int server_fd = create_socket_server();
	if (server_fd == -1) { return 1; }
	if (bind_ports(server_fd, PORT) == -1) { return 1; }
	if (listen(server_fd, BACKLOGS) == -1 ) { perror("listen"); return 1; }

	int epfd = epoll_create1(0);
	if (epfd == -1) { perror("epoll_create1"); return 1; }
	
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

	std::cout << "Server available at : http//localhost:" << PORT << "\n";

	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int n = epoll_wait(epfd, &ev, MAX_EVENTS, -1);
		
		for (int i = 0; i < n; i++) {
			int fd = events[i].data.fd;
			if (events[i].events & (EPOLLHUP | EPOLLERR)) {
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
				close(fd);
				continue;
			}

			if (fd == server_fd)
				accept_next_client(server_fd, epfd);
			else if (events[i].events & EPOLLIN)
				handle_recv(fd, epfd);
			else if (events[i].events & EPOLLOUT)
				handle_send(fd, epfd);
		}
	}

}
