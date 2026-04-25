Il y a 5 changements fondamentaux pour être 100% fidèle :

EventType enum exacte du papier
EventLoop devient un Singleton avec register_handler() et handle_events()
Les handlers s'auto-enregistrent dans leur constructeur (plus de retour de IEventHandler*)
handle_accept() retourne int, plus IEventHandler*
main() crée les handlers, plus le constructeur d'EventLoop
includes/core/IEventHandler.hpp

#pragma once

enum EventType {
    ACCEPT_EVENT  = 01,
    READ_EVENT    = 02,
    WRITE_EVENT   = 04,
    TIMEOUT_EVENT = 010,
    SIGNAL_EVENT  = 020,
    CLOSE_EVENT   = 040
};

class IEventHandler {
    public:
        virtual int getFd()           const = 0;
        virtual int handle_accept()         { return 0; }
        virtual int handle_input()          { return 0; }
        virtual int handle_output()         { return 0; }
        virtual int handle_close()          { return 0; }
        virtual ~IEventHandler() {}
};

--------

includes/core/EventLoop.hpp

#pragma once

#include "IEventHandler.hpp"
#include <vector>
#include <sys/epoll.h>

#define MAX_EVENTS 128

struct HandlerEntry {
    IEventHandler* handler;
    EventType      type;
};

class EventLoop {
    public:
        static EventLoop* instance();

        void register_handler(IEventHandler* h, EventType type);
        void remove_handler(IEventHandler* h, EventType type);
        void handle_events();

    private:
        EventLoop();
        ~EventLoop();

        int                        _epfd;
        std::vector<HandlerEntry*> _table;

        static EventLoop* _instance;
};

---------

srcs/core/EventLoop.cpp

#include "../../includes/core/EventLoop.hpp"
#include <string.h>
#include <errno.h>
#include <stdexcept>
#include <iostream>

EventLoop* EventLoop::_instance = NULL;

EventLoop* EventLoop::instance() {
    if (!_instance)
        _instance = new EventLoop();
    return _instance;
}

EventLoop::EventLoop() {
    _epfd = epoll_create1(0);
    if (_epfd == -1)
        throw std::runtime_error("epoll_create() failed: " + std::string(strerror(errno)));
}

void EventLoop::register_handler(IEventHandler* h, EventType type) {
    HandlerEntry* entry = new HandlerEntry();
    entry->handler = h;
    entry->type    = type;
    _table.push_back(entry);

    struct epoll_event ev;
    ev.events   = EPOLLIN;
    ev.data.ptr = entry;
    epoll_ctl(_epfd, EPOLL_CTL_ADD, h->getFd(), &ev);
}

void EventLoop::remove_handler(IEventHandler* h, EventType type) {
    (void)type;
    epoll_ctl(_epfd, EPOLL_CTL_DEL, h->getFd(), NULL);
    for (size_t i = 0; i < _table.size(); i++) {
        if (_table[i]->handler == h) {
            delete _table[i];
            _table.erase(_table.begin() + i);
            break;
        }
    }
}

void EventLoop::handle_events() {
    std::cout << "Serveur en écoute..." << std::endl;
    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(_epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            HandlerEntry*  entry = static_cast<HandlerEntry*>(events[i].data.ptr);
            IEventHandler* h     = entry->handler;
            if (entry->type == ACCEPT_EVENT)
                h->handle_accept();
            else if (entry->type == READ_EVENT)
                h->handle_input();
        }
    }
}

EventLoop::~EventLoop() {
    for (size_t i = 0; i < _table.size(); i++) {
        delete _table[i]->handler;
        delete _table[i];
    }
}

--------

includes/handlers/ServerHandler.hpp

#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

class ServerHandler : public IEventHandler {
    public:
        ServerHandler(addrport listen, const Server& server);
        ~ServerHandler();

        int getFd() const;
        int handle_accept();

        int  createSocket();
        void bindAddress(int serverFd, addrport listen);

    private:
        Server _server;
        int    _fd;
};

---------

srcs/handlers/ServerHandler.cpp

#include "../../includes/handlers/ServerHandler.hpp"
#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <iostream>

ServerHandler::ServerHandler(addrport addrs, const Server& server) : _server(server) {
    try {
        _fd = createSocket();
        bindAddress(_fd, addrs);
        if (listen(_fd, SOMAXCONN) == -1)
            throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
    } catch (...) {
        close(_fd);
        throw;
    }
    EventLoop::instance()->register_handler(this, ACCEPT_EVENT);
}

int ServerHandler::createSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

void ServerHandler::bindAddress(int serverFd, addrport listen) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = listen.first;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(listen.second);
    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
}

int ServerHandler::handle_accept() {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(_fd, (struct sockaddr*)&client_addr, &len);
    if (client_fd == -1)
        return -1;
    new ClientHandler(client_fd, _server);  // s'enregistre lui-même
    return 0;
}

int ServerHandler::getFd() const { return _fd; }

ServerHandler::~ServerHandler() { close(_fd); }

-----------

includes/handlers/ClientHandler.hpp

#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"

class ClientHandler : public IEventHandler {
    public:
        ClientHandler(int clientFd, const Server& server);
        ~ClientHandler();
        int getFd() const;
        int handle_input();

    private:
        int    _fd;
        Server _server;
};

----------

srcs/handlers/ClientHandler.cpp

#include "../../includes/handlers/ClientHandler.hpp"
#include "../../includes/core/EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

ClientHandler::ClientHandler(int clientFd, const Server& server) : _fd(clientFd), _server(server) {
    EventLoop::instance()->register_handler(this, READ_EVENT);
}

ClientHandler::~ClientHandler() { close(_fd); }

int ClientHandler::getFd() const { return _fd; }

int ClientHandler::handle_input() {
    char buf[4096];
    ssize_t n = recv(_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0)
        return -1;
    buf[n] = '\0';
    std::cout << "=== REQUEST ===\n" << buf << "===============" << std::endl;
    return 0;
}

-----------

main.cpp

#include "includes/core/EventLoop.hpp"
#include "includes/handlers/ServerHandler.hpp"
#include <iostream>
#include <arpa/inet.h>

int main() {
    Config c;

    Server s1;
    s1.listen.push_back(addrport(INADDR_ANY, 8080));

    Server s2;
    s2.listen.push_back(addrport(INADDR_ANY, 8081));
    s2.listen.push_back(addrport(inet_addr("127.0.0.1"), 8082));

    Server s3;
    s3.listen.push_back(addrport(inet_addr("127.0.0.1"), 9000));

    c.cfg.push_back(s1);
    c.cfg.push_back(s2);
    c.cfg.push_back(s3);

    for (size_t i = 0; i < c.cfg.size(); i++) {
        for (size_t j = 0; j < c.cfg[i].listen.size(); j++) {
            try {
                new ServerHandler(c.cfg[i].listen[j], c.cfg[i]);
            } catch (std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }

    EventLoop::instance()->handle_events();
    return 0;
}
Le point clé de la fidélité : new ServerHandler(...) dans main et new ClientHandler(...) dans handle_accept() s'enregistrent eux-mêmes dans leur constructeur via EventLoop::instance() — exactement comme Logging_Acceptor et Logging_Handler dans le papier.