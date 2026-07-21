*This project has been created as part of the 42 curriculum by avarrett, eahmeti.*

# webserv

A fully-featured, non-blocking **HTTP/1.1 server** written from scratch in **C++98** — no external libraries, no Boost. It listens on multiple ports, serves static sites, runs CGI scripts, handles file uploads, and stays responsive under load, all driven by a single `epoll` event loop built around the **Reactor** design pattern.

---

## Description

`webserv` is a 42 project whose goal is to understand how a web server actually works by rebuilding one. From the raw TCP socket up to the HTTP response, everything is implemented by hand:

- **Listen** for TCP connections on one or more `host:port` pairs;
- **Parse** raw HTTP requests (request line, headers, body, chunked transfer-encoding);
- **Route** each URL to a file, a directory listing, an upload, a CGI script, or an error page, based on an NGINX-inspired configuration file;
- **Respond** with a spec-compliant HTTP message — all without ever blocking, thanks to a single I/O multiplexing call that watches every file descriptor at once.

The server is designed around one hard rule from the subject: **it must never block, and never crash.** Every syscall, allocation, and edge case is handled so that a single misbehaving client can never freeze or take down the server.

---

## Features

**Core**
- HTTP/1.1 with **GET**, **POST** and **DELETE**
- **Non-blocking I/O** — a single `epoll_wait()` for *all* sockets and CGI pipes (listening sockets included)
- Multiple `server {}` blocks listening on **multiple ports** with different content
- NGINX-style configuration file (`server` / `location` blocks) with its own lexer, parser and validator
- **Longest-prefix** URL routing, per-location method restrictions
- Static file serving with correct **MIME types** and `Content-Length`
- **Directory listing** (autoindex) and `index` files
- **File upload** via `multipart/form-data`
- **Chunked** request decoding
- HTTP **redirections** (`return 301/302`)
- Custom **error pages** (400 → 504) with configurable `error_page`
- **Keep-alive** connections
- Request limits: header size (→ `431`), `client_max_body_size` (→ `413`)
- **Timeouts** on idle connections and long-running CGI

**CGI**
- Fully **asynchronous** CGI execution (`fork` + `execve`), integrated into the event loop
- Multiple interpreters resolved by extension (e.g. `.py` → `python3`, `.sh` → `bash`)

**Bonus**
- **Cookies and server-side sessions**
- **Multiple CGI systems** configurable per location

**Documentation**
- A set of study pages (`webserv-studies/`) documenting every concept behind the project — sockets & TCP, I/O multiplexing, the HTTP protocol, configuration, CGI, robustness, and the full architecture. They are served by the server itself and reachable at **`/webserv-studies/`** once it is running.

---

## Instructions

### Build

```sh
make
```

Compiles with `c++ -Wall -Wextra -Werror -std=c++98` into a single binary named `webserv`.

Other targets: `make clean`, `make fclean`, `make re`.

### Run

```sh
./webserv conf/default.conf
```

The program takes a single argument: the path to a configuration file. If none is passed, or the file is invalid, the server refuses to start and explains why.

Once running, open a browser at:

| URL | What you get |
|-----|--------------|
| `http://localhost:8080/` | The site root (GET only) |
| `http://localhost:8080/default/` | The in-house test suite (interactive demos for every feature) |
| `http://localhost:8080/webserv-studies/` | The project's study pages / mindmap |
| `http://localhost:8081/` | A second server on another port, with different content |

### Configuration

The config file mirrors NGINX's syntax. A minimal server looks like:

```nginx
server {
    listen 8080;                 # host:port to listen on (repeatable)
    server_name all;
    root www;                    # document root
    client_max_body_size 10m;    # request body limit

    error_page 404 /errors/404.html;

    location / {
        allowed_methods GET;     # methods allowed on this route
        index index.html;        # default file for a directory
        autoindex off;           # directory listing on/off
    }

    location /upload {
        allowed_methods GET POST DELETE;
        upload_store ./www/upload;   # where uploaded files go
    }

    location /cgi-bin {
        cgi_pass .py /usr/bin/python3;   # extension -> interpreter
        cgi_pass .sh /usr/bin/bash;
    }

    location /old-page {
        return 301 /index.html;      # redirection
    }
}
```

Supported directives: `listen`, `server_name`, `root`, `index`, `client_max_body_size`, `error_page`, `autoindex`, `allowed_methods`, `upload_store`, `cgi_pass`, `return`. Most can be set at the `server` level and are inherited by every `location` that does not override them.

---

## Architecture — a bird's-eye view

The whole server is built on **one idea**: a single loop watches every file descriptor, and whenever one is ready, it hands the event to the object responsible for that descriptor. That object decides what to do. The loop itself knows nothing about HTTP, CGI, or sockets — it only knows the *interface*. This is the **Reactor** pattern (Schmidt, 1995), and polymorphism is what makes it work.

### The Reactor: one loop, many handlers

```
                         ┌─────────────────────────────────────────┐
                         │         EventLoop  (singleton)          │
                         │        "Initiation Dispatcher"          │
                         │                                         │
                         │      one epoll_wait() for ALL fds       │
                         │   sockets + client conns + CGI pipes    │
                         └───────────────────┬─────────────────────┘
                                             │
                     for each ready event:   │  entry->handler->handle_input()
                     dispatch polymorphically│  (or handle_accept / handle_output)
                                             │
                    ┌────────────────────────┼─────────────────────────┐
                    ▼                        ▼                         ▼
            ┌───────────────┐        ┌───────────────┐         ┌───────────────┐
            │ ServerHandler │        │ ClientHandler │         │  CGI handlers │
            │  (a listen)   │        │ (a connection)│         │ read / write  │
            └───────┬───────┘        └───────┬───────┘         └───────────────┘
                    │                        │
             accept() → new           recv / parse / route
             ClientHandler            build response / send
```

Every handler implements the same tiny contract:

```cpp
class IEventHandler {
    virtual int  getFd()          const = 0;
    virtual int  handle_accept()  { return 0; }
    virtual int  handle_input()   { return 0; }   // fd readable  (EPOLLIN)
    virtual int  handle_output()  { return 0; }   // fd writable  (EPOLLOUT)
    virtual int  handle_timeout() { return 0; }
    // ...
};
```

Because `EventLoop` only ever holds `IEventHandler*` pointers, **it never needs a `switch` or a cast**. When `epoll_wait()` returns, the loop reads back the handler pointer it stored in `ev.data.ptr` and calls the right virtual method — the correct concrete handler runs automatically. Adding a brand-new kind of handler means implementing the interface and registering it; the loop's code never changes.

A **single convention** governs cleanup: any handler that returns `-1` tells the loop to unregister and `delete` it. Its destructor closes the fd (RAII). No fd is ever leaked, and there is exactly one place where handlers die.

### The life of a request

```
  ┌──────┐   1        2         3        4         5          6        7
  │client│ ──────►  epoll  ──► recv  ──► parse ──► route  ──► build ──► send
  └──────┘         _wait()      +         (state    (config    the       +
                              accumulate   machine)  →loc)    response  keep-alive
                                                       │
                                        ┌──────────────┼───────────────┐
                                        ▼              ▼               ▼
                                   StaticHandler   CGIHandler     ErrorHandler
                                   GET/POST/DELETE  fork+execve    400 → 504
                                   file / dir /     (async)
                                   upload
```

1. **`ServerHandler`** owns one listening socket per `listen` directive (`socket → SO_REUSEADDR → O_NONBLOCK → FD_CLOEXEC → bind → listen`), and registers *itself* with the loop. On a connection it `accept()`s and spawns a `ClientHandler`.
2. **`ClientHandler`** does one non-blocking `recv()` per event and feeds the bytes to the **`HttpParser`**, a state machine (`HEADERS → BODY / CHUNKED → COMPLETE / ERROR`) that turns raw text into an `HttpRequest`. TCP delivers data in fragments, so the parser accumulates across multiple reads.
3. Once the request is `COMPLETE`, the **`Router`** performs longest-prefix matching against the config, then runs the guard rails in order: method allowed? redirect? path resolution (with directory-traversal protection → `403`)? file exists (`404`)? Finally it decides between a static handler, an upload handler, a CGI handler, or an error handler.

### Delegation through a common base — `ARequestHandler`

All response producers inherit from an abstract base, **`ARequestHandler`**, which owns the shared logic: building the final HTTP message (status line, headers, `Content-Length`, `Connection`, cookies…) and switching the client to `WRITE_EVENT`. Each concrete handler only implements *what makes it different*:

```
                 ┌───────────────────────┐
                 │    ARequestHandler    │   buildResponse(), common headers
                 └───────────┬───────────┘
        ┌──────────────┬─────┴───────┬──────────────────┐
        ▼              ▼             ▼                  ▼
  StaticHandler  MultipartHandler  ErrorHandler     CGIHandler
  read/write/    file uploads      error pages      launches the CGI
  unlink files   (multipart)       (custom/default) (asynchronous)
```

### Asynchronous CGI — the Observer pattern

A CGI script must not block the loop while it runs. So `CGIHandler` **forks the process, wires two pipes, and returns immediately** — the client is *not* answered yet. The two pipe ends become their own event handlers (`CGIWriteHandler` to feed the request body in, `CGIReadHandler` to read the script's output), and they join the same `epoll`.

When the script finishes, the read handler notifies the client through a narrow interface, **`IResponseSink`** (the Observer pattern):

```cpp
class IResponseSink {
    virtual void onCgiDone(const std::string& rawHttpResp) = 0;
    virtual void onCgiStart(CGIReadHandler* rd) { (void)rd; }
};
```

The CGI handler only knows `IResponseSink` — never `ClientHandler` directly. This avoids a circular dependency, and the `onCgiStart` / `onCgiDone` handshake lets the two objects **detach cleanly** if either dies first (client disconnect, CGI timeout), preventing any dangling pointer / use-after-free. The client's `onCgiDone()` simply stores the response and flips itself to `WRITE_EVENT` — back into the normal send path.

### The config pipeline

Loading a config is a three-stage pipeline: **`Lexer`** (tokenize `{ } ; words`) → **`Parser`** (build `Server` / `Location` structs) → **`Validator`** (check coherence and propagate `server`-level directives down into each `location`). After propagation, the Router only ever consults the matched `Location` — a clean separation between *parsing the config* and *serving requests*.

> Want the deep dive? Every one of these concepts has its own illustrated study page, served by the server at **`/webserv-studies/`**.

---

## Testing

The project ships with several test layers (see `tests/`):

```sh
./webserv conf/default.conf &          # start the server first

bash tests/run_all_tests.sh            # methods, status codes, chunked,
                                       # malformed requests, keep-alive, multi-port
bash tests/bonus_cookies_sessions.sh   # bonus: cookies + server sessions
bash tests/bonus_multi_cgi.sh          # bonus: Python (.py) + Bash (.sh) CGI
python3 tests/stress_test.py           # stress: 500 requests over 50 threads
```

All scripts accept an optional `host:port` argument (default `localhost:8080`).

In the browser, `http://localhost:8080/default/` offers an interactive test suite: every page (GET, POST, upload with progress, DELETE, CGI, error pages, stress, bonus) has a **"run everything"** button plus manual demos.

The official 42 tester is also supported:

```sh
./tester http://localhost:8080
```

> **Note on the last stress test:** on the school computers, the final stress test is too heavy for the available hardware and can freeze the machine — it still completes, but it takes about **45 minutes** if you let it run to the end. On a slightly more powerful machine (e.g. a personal VPS) it runs smoothly. **In every case, all tests pass**, school computers included; you just need the patience to wait it out.

---

## Resources

A curated selection of the most useful references for each area of the project. Start with the RFCs and man pages, then go deeper with the guides. (Every one of these is also linked from the study pages under `/webserv-studies/`.)

### RFCs & Standards

- **[RFC 7230](https://datatracker.ietf.org/doc/html/rfc7230)** — HTTP/1.1 Message Syntax and Routing: the exact structure of HTTP requests and responses.
- **[RFC 7231](https://datatracker.ietf.org/doc/html/rfc7231)** — HTTP/1.1 Semantics and Content: methods, status codes, content headers.
- **[RFC 3875](https://datatracker.ietf.org/doc/html/rfc3875)** — The Common Gateway Interface (CGI) 1.1: environment variables and the CGI communication protocol.

### Essential man pages

- **Sockets** — `man 2 socket, bind, listen, accept, recv, send, setsockopt, getsockname`
- **I/O multiplexing** — `man 7 epoll`, `man 2 epoll_create1, epoll_ctl, epoll_wait`
- **Processes & pipes** — `man 2 fork, execve, pipe, dup2, waitpid, kill, signal`
- **DNS resolution** — `man 3 getaddrinfo, freeaddrinfo, gai_strerror`

### Guides & tutorials

- **[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)** — *the* reference for network programming in C: the whole socket lifecycle with concrete examples.
- **[NGINX documentation](https://nginx.org/en/docs/)** — to understand the configuration format and compare your server's HTTP behaviour.
- **[MDN — HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)** — clear, modern documentation of the HTTP protocol: methods, headers, status codes, MIME types.
- **The Linux Programming Interface** (Michael Kerrisk) — the syscall bible; chapters on sockets, poll/epoll, and fork/exec.

### Design pattern

- **D. C. Schmidt, "Reactor: An Object Behavioral Pattern for Concurrent Event Demultiplexing and Dispatching" (1995)** — the pattern our `EventLoop` / `IEventHandler` design is based on.

### Use of AI

AI assistance was used for a focused set of supporting tasks, **not** for the core C++ implementation (the event loop, HTTP parsing, routing, CGI and configuration logic were written by us):

- **Understanding concepts** — clarifying epoll semantics, the Reactor pattern, chunked encoding, multipart parsing and CGI internals while learning the topics.
- **Writing the study pages** — drafting the HTML and CSS of the `webserv-studies/` documentation and the in-house test pages under `www/default/`.
- **Tests** — helping create the functional test scripts and the multi-threaded stress test used to validate robustness under load.
