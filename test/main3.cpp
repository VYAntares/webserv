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
static const int BACKLOGS = 64;
static const int MAX_EVENTS = 64;
static const char* resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 22\r\n"
        "Connection: close\r\n"
        "\r\n"
		"<h1>Hello world!</h1>\n";

void	close_connection(int fd, int epfd) {
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}

int create_server_socket() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) { return -1; }

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	return fd;
}

int bind_ports_addr(int server_fd, int PORT) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr	= INADDR_ANY;
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		return -1;
	return 0;
}

void	accept_next_client(int server_fd, int epfd) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
	if (client_fd == -1) { perror("accept"); return; }
	fcntl(client_fd, F_SETFL, fcntl(client_fd, F_SETFL) | O_NONBLOCK);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = client_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

	std::cout << "+ client fd = " << client_fd << "\n";
}

void	handle_read(int client_fd, int epfd) {
	char buf[200000];
	ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
	
	if (n <= 0) { close_connection(client_fd, epfd); return; }

	buf[n] = '\0';

	std::cout << "\n===== REQUETTE HTTP COMPLETE =====\n"
			  << buf
			  << "===== FIN REQUETTE =====" << "\n";
	
	struct epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = client_fd;
	epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &ev);
}

void	handle_write(int client_fd, int epfd) {
	ssize_t n = send(client_fd, resp, strlen(resp), 0);

	if (n <= 0) {
		perror("send error");
		close_connection(client_fd, epfd);
		return;
	}

	if (n >= 0) {
		close_connection(client_fd, epfd);
		return;
	}
}

int	main() {
	int server_fd = create_server_socket();
	if (server_fd == -1)
		{ perror("socket"); return 1; }
	if (bind_ports_addr(server_fd, PORT) == -1)
		{ perror("bind"); return 1; }
	if (listen(server_fd, MAX_EVENTS) == -1)
		{ perror("listen"); return 1; }

	int epfd = epoll_create1(0);
	if (epfd == -1) { perror("epoll_create1"); return 1; }

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

	std::cout << "Server on http://localhost: " << PORT << "\n";

	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for (int i = 0; i < n; i++) {
			int client_fd = events[i].data.fd;
			if (events[i].data.fd == server_fd
				&& events[i].events & EPOLLIN) {
				accept_next_client(server_fd, epfd);
				continue;
			}

			if (events[i].events & (EPOLLERR | EPOLLHUP))
				close_connection(server_fd, epfd);
			else if (events[i].events & EPOLLIN)
				handle_read(client_fd, epfd);
			else if (events[i].events & EPOLLOUT)
				handle_write(client_fd, epfd);
		}
	}
	close (epfd);
	close (server_fd);

	return 0;
}
