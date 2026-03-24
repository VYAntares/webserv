#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include <cstdio>    // perror
#include <cstdlib>   // strtol
#include <sstream>   // ostringstream
#include <iostream>
#include <map>
#include <string>

static const int PORT       = 8080;
static const int BACKLOG    = 10;
static const int MAX_EVENTS = 64;

// ── État par connexion ─────────────────────────────────
struct Connection {
    int         fd;
    std::string request;
    std::string response;
    size_t      sent;
};

std::map<int, Connection*> connections;

// ── Fermer proprement ─────────────────────────────────
void close_connection(Connection* conn, int epfd) {
    std::cout << "[DEBUG] close fd=" << conn->fd << " (request was " << conn->request.size() << " bytes)\n";
    epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, NULL);
    close(conn->fd);
    connections.erase(conn->fd);
    delete conn;
}

// ── Construire une réponse HTTP simple ───────────────
std::string build_response(const std::string& /*request*/) {  // fix: unused param
    std::string body = "<h1>Hello World!</h1>\n";
    // fix: std::to_string n'existe pas en C++98 → ostringstream
    std::ostringstream oss;
    oss << body.size();
    std::string resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + oss.str() + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;
    return resp;
}

// ── Crée le socket serveur ────────────────────────────
int create_server_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

// ── Bind IP:port ──────────────────────────────────────
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

// ── Accept : crée une Connection, enregistre dans epoll ─
void accept_next_client(int server_fd, int epfd) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd,
                           (struct sockaddr*)&client_addr, &len);
    if (client_fd == -1) { perror("accept"); return; }

    fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);

    Connection* conn = new Connection();
    conn->fd   = client_fd;
    conn->sent = 0;
    connections[client_fd] = conn;

    struct epoll_event ev;
    ev.events   = EPOLLIN;
    ev.data.ptr = conn;
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

    std::cout << "[+] client fd=" << client_fd
              << " from " << inet_ntoa(client_addr.sin_addr) << "\n";
}

// ── Lecture : UN recv(), accumule, bascule si complet ──
void handle_read(Connection* conn, int epfd) {
    char buf[4096];
    ssize_t n = recv(conn->fd, buf, sizeof(buf) - 1, 0);

    std::cout << "[DEBUG] recv() fd=" << conn->fd << " returned " << n
              << " (total accumulated: " << conn->request.size() + (n > 0 ? n : 0) << " bytes)\n";

    if (n <= 0) {
        std::cout << "[DEBUG] fd=" << conn->fd << (n == 0 ? " client disconnected (EOF)" : " recv error") << "\n";
        close_connection(conn, epfd);
        return;
    }

    buf[n] = '\0';
    conn->request += buf;

    if (conn->request.find("\r\n\r\n") != std::string::npos) {
        std::cout << "\n===== REQUETE HTTP COMPLETE (fd=" << conn->fd << ") =====\n"
                  << conn->request
                  << "===== FIN REQUETE (" << conn->request.size() << " bytes) =====\n\n";

        conn->response = build_response(conn->request);
        conn->sent = 0;

        std::cout << "[DEBUG] fd=" << conn->fd << " -> bascule EPOLLIN -> EPOLLOUT"
                  << " (response: " << conn->response.size() << " bytes)\n";

        struct epoll_event ev;
        ev.events   = EPOLLOUT;
        ev.data.ptr = conn;
        epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
    } else {
        std::cout << "[DEBUG] fd=" << conn->fd << " headers pas encore complets, on attend...\n";
    }
}

// ── Écriture : UN send(), met à jour sent, ferme si fini ─
void handle_write(Connection* conn, int epfd) {
    ssize_t n = send(conn->fd,
                     conn->response.c_str() + conn->sent,
                     conn->response.size() - conn->sent,
                     0);

    std::cout << "[DEBUG] send() fd=" << conn->fd << " returned " << n
              << " (sent " << conn->sent + (n > 0 ? n : 0) << "/" << conn->response.size() << ")\n";

    if (n <= 0) {
        std::cout << "[DEBUG] fd=" << conn->fd << " send error\n";
        close_connection(conn, epfd);
        return;
    }

    conn->sent += n;

    if (conn->sent >= conn->response.size()) {
        std::cout << "[DEBUG] fd=" << conn->fd << " response complete, closing\n";
        close_connection(conn, epfd);
    }
}

// ── La boucle événementielle ────────────────────────────
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
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    std::cout << "Serveur sur http://localhost:" << PORT << "\n";
    std::cout << "[DEBUG] server_fd=" << server_fd << " epfd=" << epfd << " en attente...\n";

    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {

            if (events[i].data.fd == server_fd
                && (events[i].events & EPOLLIN)) {
                accept_next_client(server_fd, epfd);
                continue;
            }

            Connection* conn = (Connection*)events[i].data.ptr;

            if (events[i].events & (EPOLLERR | EPOLLHUP))
                close_connection(conn, epfd);
            else if (events[i].events & EPOLLIN)
                handle_read(conn, epfd);
            else if (events[i].events & EPOLLOUT)
                handle_write(conn, epfd);
        }
    }

    close(epfd);
    close(server_fd);
    return 0;
}