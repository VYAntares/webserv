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

static const int PORT    = 8080;
static const int BACKLOG = 10;
static const int MAX_EVENTS = 64;

const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 22\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<h1>Hello World!</h1>\n";

// ── Étape 1 : crée le socket TCP/IPv4 ─────────────────────────
int create_server_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); return -1; }
	
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

    return fd;
}

// ── Étape 2 : associe IP:port au socket ───────────────────────
int bind_address(int fd, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		{ perror("bind"); return -1; }
    return 0;
}

// ── Étape 4 : accepte UN client de la file d'attente ──────────
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
	epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

	std::cout << "[+] client fd=" << client_fd
              << " from " << inet_ntoa(client_addr.sin_addr) << "\n";
}

// ── Lecture : EPOLLIN → recv, puis passe en EPOLLOUT ──
void handle_read(int fd, int epfd) {
    char buf[4096];
	ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);

	if (n <= 0) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return;
	}

	buf[n] = '\0';
	std::cout << buf;

	struct epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

void handle_write(int fd, int epfd) {
    send(fd, response, strlen(response), 0);

	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	close (fd);
}

// ── main : assemble les 4 étapes ──────────────────────────────
int main() {
	signal(SIGPIPE, SIG_IGN);

    int server_fd = create_server_socket();
    if (server_fd == -1) return 1;
    if (bind_address(server_fd, PORT) == -1) return 1;
    if (listen(server_fd, BACKLOG) == -1)
        { perror("listen"); return 1; }

	int epfd = epoll_create1(0);
	if (epfd == -1) { perror("epoll_create1"); return 1; }

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    std::cout << "Serveur sur http://localhost:" << PORT << "\n";

	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

		for (int i = 0; i < n; i++) {
			int fd = events[i].data.fd;
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				close(fd);
				continue;
			}

			if (fd == server_fd)
				accept_next_client(server_fd, epfd);
			else if (events[i].events & EPOLLIN)
				handle_read(fd, epfd);
			else if (events[i].events & EPOLLOUT)
				handle_write(fd, epfd);
		}
    }
	
	close(epfd);
	close(server_fd);
    return 0;
}
