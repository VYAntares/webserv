#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <cctype>   // isxdigit

static const int PORT       = 8080;
static const int BACKLOG    = 10;
static const int MAX_EVENTS = 64;

// ─────────────────────────────────────────────────────────────
//  États de la state-machine par connexion
// ─────────────────────────────────────────────────────────────
enum ConnState {
    STATE_READING_HEADERS,   // accumulation jusqu'à \r\n\r\n
    STATE_READING_BODY_CL,   // lecture body par Content-Length
    STATE_READING_BODY_CHUNKED, // lecture body chunked
    STATE_DRAINING,          // on vide le recv buffer avant close
    STATE_WRITING            // on envoie la réponse
};

struct Connection {
    int         fd;
    ConnState   state;

    std::string raw;        // tout ce qui est reçu (headers + body brut)
    size_t      header_end; // position de fin des headers dans raw (\r\n\r\n + 4)

    // Body
    bool        is_chunked;
    ssize_t     content_length; // -1 = pas encore connu
    std::string body;           // body décodé (chunked ou raw)

    // Réponse
    std::string response;
    size_t      sent;
};

std::map<int, Connection*> connections;

// ─────────────────────────────────────────────────────────────
//  Helpers de parsing
// ─────────────────────────────────────────────────────────────

// Cherche un header (insensible à la casse) et retourne sa valeur
std::string find_header(const std::string& headers, const std::string& name) {
    std::string lower_headers = headers;
    std::string lower_name    = name;
    for (size_t i = 0; i < lower_headers.size(); i++)
        lower_headers[i] = std::tolower(lower_headers[i]);
    for (size_t i = 0; i < lower_name.size(); i++)
        lower_name[i] = std::tolower(lower_name[i]);

    size_t pos = lower_headers.find(lower_name);
    if (pos == std::string::npos) return "";
    pos = headers.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < headers.size() && headers[pos] == ' ') pos++;
    size_t end = headers.find("\r\n", pos);
    if (end == std::string::npos) end = headers.size();
    return headers.substr(pos, end - pos);
}

// Décode un body chunked (RFC 7230 §4.1)
// Retourne true si le chunk terminal "0\r\n\r\n" a été trouvé et le body décodé
bool decode_chunked(const std::string& raw_body, std::string& out_body) {
    out_body.clear();
    size_t pos = 0;
    while (pos < raw_body.size()) {
        // Lire la taille du chunk (hex) jusqu'à \r\n
        size_t crlf = raw_body.find("\r\n", pos);
        if (crlf == std::string::npos) return false; // chunk size pas encore complet

        std::string hex = raw_body.substr(pos, crlf - pos);
        // Ignorer les extensions de chunk (après ';')
        size_t semi = hex.find(';');
        if (semi != std::string::npos) hex = hex.substr(0, semi);

        // Vérification : que des hex digits
        for (size_t i = 0; i < hex.size(); i++) {
            if (!std::isxdigit((unsigned char)hex[i])) return false;
        }

        char* endptr;
        unsigned long chunk_size = std::strtoul(hex.c_str(), &endptr, 16);

        pos = crlf + 2; // avancer après \r\n

        if (chunk_size == 0) {
            // Chunk terminal : doit être suivi de \r\n (trailers ignorés ici)
            return true; // body complet
        }

        // Vérifier qu'on a assez de données
        if (pos + chunk_size + 2 > raw_body.size())
            return false; // chunk data pas encore arrivée

        out_body.append(raw_body, pos, chunk_size);
        pos += chunk_size + 2; // avancer après les données + \r\n
    }
    return false; // chunk terminal pas encore vu
}

// ─────────────────────────────────────────────────────────────
//  Construction de la réponse HTTP
// ─────────────────────────────────────────────────────────────
std::string build_response(const std::string& body_decoded) {
    std::ostringstream oss;
    oss << "<h1>Hello World!</h1>\n"
        << "<p>Body received: " << body_decoded.size() << " bytes</p>\n";
    std::string resp_body = oss.str();

    std::ostringstream len;
    len << resp_body.size();

    return std::string("HTTP/1.1 200 OK\r\n")
         + "Content-Type: text/html\r\n"
         + "Content-Length: " + len.str() + "\r\n"
         + "Connection: close\r\n"
         + "\r\n"
         + resp_body;
}

// ─────────────────────────────────────────────────────────────
//  Fermeture PROPRE : shutdown(SHUT_WR) + draining + close
//
//  On ne close() pas directement si le peer a encore des données
//  à envoyer → ça provoquerait un RST.
//  On shutdown(SHUT_WR) d'abord (envoie FIN), on passe en STATE_DRAINING
//  pour vider le recv-buffer, puis on close().
// ─────────────────────────────────────────────────────────────
void begin_drain(Connection* conn, int epfd) {
    std::cout << "[DEBUG] fd=" << conn->fd << " -> shutdown(SHUT_WR), draining\n";
    shutdown(conn->fd, SHUT_WR); // envoie FIN proprement
    conn->state = STATE_DRAINING;

    struct epoll_event ev;
    ev.events   = EPOLLIN; // on surveille les données restantes à jeter
    ev.data.ptr = conn;
    epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
}

void close_connection(Connection* conn, int epfd) {
    std::cout << "[DEBUG] close fd=" << conn->fd << "\n";
    epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, NULL);
    close(conn->fd);
    connections.erase(conn->fd);
    delete conn;
}

// ─────────────────────────────────────────────────────────────
//  Crée le socket serveur
// ─────────────────────────────────────────────────────────────
int create_server_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

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

// ─────────────────────────────────────────────────────────────
//  Accept
// ─────────────────────────────────────────────────────────────
void accept_next_client(int server_fd, int epfd) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
    if (client_fd == -1) { perror("accept"); return; }

    fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);

    Connection* conn    = new Connection();
    conn->fd            = client_fd;
    conn->state         = STATE_READING_HEADERS;
    conn->header_end    = 0;
    conn->is_chunked    = false;
    conn->content_length = -1;
    conn->sent          = 0;
    connections[client_fd] = conn;

    struct epoll_event ev;
    ev.events   = EPOLLIN;
    ev.data.ptr = conn;
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

    std::cout << "[+] client fd=" << client_fd
              << " from " << inet_ntoa(client_addr.sin_addr) << "\n";
}

// ─────────────────────────────────────────────────────────────
//  Passer en écriture (réponse prête)
// ─────────────────────────────────────────────────────────────
void switch_to_write(Connection* conn, int epfd) {
    conn->response = build_response(conn->body);
    conn->sent     = 0;
    conn->state    = STATE_WRITING;

    std::cout << "[DEBUG] fd=" << conn->fd
              << " -> EPOLLOUT (response " << conn->response.size() << " bytes)\n";

    struct epoll_event ev;
    ev.events   = EPOLLOUT;
    ev.data.ptr = conn;
    epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev);
}

// ─────────────────────────────────────────────────────────────
//  Lecture : state machine complète
// ─────────────────────────────────────────────────────────────
void handle_read(Connection* conn, int epfd) {
    char buf[200000];
    ssize_t n = recv(conn->fd, buf, sizeof(buf), 0);

    std::cout << "[DEBUG] recv() fd=" << conn->fd << " returned " << n << "\n";

    // ── Draining : on jette et on attend EOF ──────────────────
    if (conn->state == STATE_DRAINING) {
        if (n <= 0) {
            std::cout << "[DEBUG] fd=" << conn->fd << " drain complete, closing\n";
            close_connection(conn, epfd);
        }
        // n > 0 : on continue de drainer (ne rien faire)
        return;
    }

    if (n <= 0) {
        std::cout << "[DEBUG] fd=" << conn->fd
                  << (n == 0 ? " EOF" : " recv error") << "\n";
        close_connection(conn, epfd);
        return;
    }

    conn->raw.append(buf, n);

    // ── Étape 1 : attendre la fin des headers ─────────────────
    if (conn->state == STATE_READING_HEADERS) {
        size_t pos = conn->raw.find("\r\n\r\n");
        if (pos == std::string::npos) {
            std::cout << "[DEBUG] fd=" << conn->fd << " headers pas complets\n";
            return;
        }
        conn->header_end = pos + 4;

        // Parser les headers
        std::string headers = conn->raw.substr(0, pos);
        std::cout << "\n===== HEADERS (fd=" << conn->fd << ") =====\n"
                  << headers << "\n========================\n\n";

        std::string te  = find_header(headers, "Transfer-Encoding");
        std::string cl  = find_header(headers, "Content-Length");

        if (te.find("chunked") != std::string::npos) {
            conn->is_chunked     = true;
            conn->content_length = -1;
            conn->state          = STATE_READING_BODY_CHUNKED;
            std::cout << "[DEBUG] fd=" << conn->fd << " mode: chunked\n";
        } else if (!cl.empty()) {
            char* ep;
            conn->content_length = (ssize_t)std::strtol(cl.c_str(), &ep, 10);
            conn->is_chunked     = false;
            conn->state          = STATE_READING_BODY_CL;
            std::cout << "[DEBUG] fd=" << conn->fd
                      << " mode: Content-Length=" << conn->content_length << "\n";
        } else {
            // Pas de body (GET, DELETE, etc.)
            std::cout << "[DEBUG] fd=" << conn->fd << " mode: no body\n";
            switch_to_write(conn, epfd);
            return;
        }
    }

    // ── Étape 2a : body par Content-Length ───────────────────
    if (conn->state == STATE_READING_BODY_CL) {
        size_t body_received = conn->raw.size() - conn->header_end;
        std::cout << "[DEBUG] fd=" << conn->fd
                  << " body: " << body_received << "/" << conn->content_length << "\n";

        if ((ssize_t)body_received >= conn->content_length) {
            conn->body = conn->raw.substr(conn->header_end,
                                          (size_t)conn->content_length);
            std::cout << "[DEBUG] fd=" << conn->fd << " body complet ("
                      << conn->body.size() << " bytes)\n";
            switch_to_write(conn, epfd);
        }
        return;
    }

    // ── Étape 2b : body chunked ───────────────────────────────
    if (conn->state == STATE_READING_BODY_CHUNKED) {
        std::string raw_body = conn->raw.substr(conn->header_end);
        std::string decoded;
        if (decode_chunked(raw_body, decoded)) {
            conn->body = decoded;
            std::cout << "[DEBUG] fd=" << conn->fd
                      << " chunked body complet (" << conn->body.size() << " bytes décodés)\n";
            switch_to_write(conn, epfd);
        } else {
            std::cout << "[DEBUG] fd=" << conn->fd
                      << " chunked body pas encore complet (" << raw_body.size() << " bytes bruts)\n";
        }
        return;
    }
}

// ─────────────────────────────────────────────────────────────
//  Écriture
// ─────────────────────────────────────────────────────────────
void handle_write(Connection* conn, int epfd) {
    ssize_t n = send(conn->fd,
                     conn->response.c_str() + conn->sent,
                     conn->response.size()  - conn->sent,
                     0);

    std::cout << "[DEBUG] send() fd=" << conn->fd << " returned " << n
              << " (" << conn->sent + (n > 0 ? n : 0)
              << "/" << conn->response.size() << ")\n";

    if (n <= 0) {
        std::cout << "[DEBUG] fd=" << conn->fd << " send error\n";
        close_connection(conn, epfd);
        return;
    }

    conn->sent += n;

    if (conn->sent >= conn->response.size()) {
        std::cout << "[DEBUG] fd=" << conn->fd << " réponse complète → drain\n";
        // Fermeture PROPRE : on ne close() pas directement
        begin_drain(conn, epfd);
    }
}

// ─────────────────────────────────────────────────────────────
//  Boucle événementielle
// ─────────────────────────────────────────────────────────────
int main() {
    signal(SIGPIPE, SIG_IGN);

    int server_fd = create_server_socket();
    if (server_fd == -1) return 1;
    if (bind_address(server_fd, PORT) == -1) return 1;
    if (listen(server_fd, BACKLOG) == -1) { perror("listen"); return 1; }

    int epfd = epoll_create1(0);
    if (epfd == -1) { perror("epoll_create1"); return 1; }

    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    std::cout << "Serveur sur http://localhost:" << PORT << "\n";
    std::cout << "[DEBUG] server_fd=" << server_fd << " epfd=" << epfd << "\n";

    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {

            // Nouvelle connexion
            if (events[i].data.fd == server_fd
                && (events[i].events & EPOLLIN)) {
                accept_next_client(server_fd, epfd);
                continue;
            }

            Connection* conn = (Connection*)events[i].data.ptr;

            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                std::cout << "[DEBUG] fd=" << conn->fd << " EPOLLERR/HUP\n";
                close_connection(conn, epfd);
            } else if (events[i].events & EPOLLIN) {
                handle_read(conn, epfd);
            } else if (events[i].events & EPOLLOUT) {
                handle_write(conn, epfd);
            }
        }
    }

    close(epfd);
    close(server_fd);
    return 0;
}
