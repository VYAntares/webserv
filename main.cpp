#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <stdio.h>

static const int PORT    = 8080;
static const int BACKLOG = 10;

// ── Étape 1 : crée le socket TCP/IPv4 ──────────────────────────
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

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind");
		return -1;
	}
    return 0;
}

// ── Étape 4 : accepte UN client de la file d'attente ──────────
int accept_next_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd,
                           (struct sockaddr*)&client_addr, &len);
    if (client_fd == -1) {
		if (errno == EAGAIN) return -2;

		perror("accept"); return -1;
	}

    std::cout << "[+] client fd=" << client_fd
              << " from " << inet_ntoa(client_addr.sin_addr) << "\n";
	
	fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);

    return client_fd;
}

// ── Étape 5 : lit la requête, envoie la réponse ───────────────
void handle_client(int client_fd) {
    char buf[4096];
	int n;

    do {
        n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    } while (n == -1 && errno == EAGAIN);  // retry jusqu'à avoir quelque chose

	if (n == -1 && errno == EAGAIN) { return; }
    if (n <= 0) { close(client_fd); return; }
    buf[n] = '\0';
    std::cout << buf;

    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"  // ← text/html, pas text
        "Content-Length: 22\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<h1>Hello World!</h1>\n";

    send(client_fd, response, strlen(response), 0);
    close(client_fd);
}

// ── main : assemble les 4 étapes ──────────────────────────────
int main() {
    int server_fd = create_server_socket();
    if (server_fd == -1) return 1;

    if (bind_address(server_fd, PORT) == -1) return 1;

    if (listen(server_fd, BACKLOG) == -1)
        { perror("listen"); return 1; }

    std::cout << "Serveur sur http://localhost:" << PORT << "\n";

    // Boucle principale — non-bloquante, mais toujours séquentielle (busy-wait)
    // → accept() et recv() retournent EAGAIN au lieu de bloquer
    // → CPU = 100% même sans trafic (busy-wait) — prochaine étape : epoll_wait()

	while (true) {
        int client_fd = accept_next_client(server_fd);
        if (client_fd == -1 || client_fd == -2) 
			continue;
        handle_client(client_fd);
    }
    return 0;
}
