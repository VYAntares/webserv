# srcs/handlers/ — Résumé & TODO

## Rôle dans le projet

Ce dossier contient toutes les **classes concrètes qui implémentent `IEventHandler`** : ce sont les "Concrete Event Handlers" du pattern Reactor. Chacune représente **un type de fd surveillé** par `EventLoop` et sait quoi faire quand cet fd devient prêt.

| Handler            | Quand instancié                                      | Type de fd surveillé              | Rôle                                                                       |
| ------------------ | ---------------------------------------------------- | --------------------------------- | -------------------------------------------------------------------------- |
| `ServerHandler`    | Au démarrage, un par paire `(addr, port)` du config  | Socket TCP listen                 | `accept()` une connexion entrante, crée un `ClientHandler`.                |
| `ClientHandler`    | À chaque accept                                      | Socket TCP client                 | Pilote le cycle parse → route → response d'une connexion.                  |
| `CGIWriteHandler`  | Quand on lance un CGI avec body (POST)               | Pipe stdin du processus CGI       | Pousse le body de la requête vers stdin du CGI, non bloquant.              |
| `CGIReadHandler`   | Quand on lance un CGI                                | Pipe stdout du processus CGI      | Lit la sortie du CGI jusqu'à EOF, signale au `CGIHandler` qu'elle est prête. |
| `TimerHandler`     | À chaque accept + à chaque démarrage CGI             | (potentiellement `timerfd`)       | Coupe une connexion ou un CGI bloqué au bout de N secondes.                |

### Découpage SOLID

- **SRP** : chaque classe gère **un seul type de fd** et **un seul cycle de vie**. `ServerHandler` ne sait rien d'HTTP. `ClientHandler` ne sait rien de socket BSD bas niveau (déjà accepté). `CGI*Handler` ne sait rien d'HTTP, juste de pipes et de pid.
- **OCP** : ajouter un handler (SignalHandler, FileWatcher, etc.) = nouvelle classe qui hérite de `IEventHandler`, sans modifier `EventLoop` ni les autres handlers.
- **DIP** : `ClientHandler` ne dépend que de `IRequestHandler*` (abstraction http), pas de `StaticHandler` ou `CGIHandler` concret. C'est le `Router` qui injecte le bon.

---

## État actuel

### `ServerHandler` ([ServerHandler.cpp](ServerHandler.cpp))
**Fait.**
- Constructeur : `createSocket()` → `bindAddress()` → `listen()` → `register_handler(this, ACCEPT_EVENT)`.
- `createSocket` : `socket(AF_INET, SOCK_STREAM, 0)`, `SO_REUSEADDR`, `O_NONBLOCK` via `fcntl(F_SETFL, ...)`.
- `bindAddress` : `htons(port)`, addr déjà au bon format réseau (parser fait le boulot).
- `handle_accept` : `accept()`, `new ClientHandler(client_fd, _server, client_addr)` (le ClientHandler s'enregistre lui-même dans son ctor), `client_addr` transmis.
- `printServer()` : affiche joliment la config résolue (debug).
- RAII : try/catch dans le ctor referme le fd en cas d'échec ; destructeur `close(_fd)`.

### `ClientHandler` ([ClientHandler.cpp](ClientHandler.cpp))
**Partiellement fait — fonctionne pour un cas simple.**
- Constructeur : `register_handler(this, READ_EVENT)` immédiatement après accept. Stocke `_peerAddr` via `_buildPeerStr()` et initialise `_lastActivity`.
- `handle_input` : `recv()` 4096 octets, push dans `HttpParser`, si `COMPLETE` → `_handleComplete()`, si `ERROR` → `_handleError()`.
- `_handleComplete` : détermine `_keepAlive` (HTTP/1.1 keep-alive par défaut, respect du header `Connection`), `Router::route()` → `buildResponse()`, switch en `WRITE_EVENT`.
- `_handleError` : force `_keepAlive = false`, route l'erreur, switch en `WRITE_EVENT`.
- `handle_output` : `send()` avec `MSG_NOSIGNAL`, gère le send partiel via `_sent`. Si fini et keep-alive → `_reset()` + READ_EVENT ; sinon retourne -1 (Connection: close).
- `_reset()` : `delete _rh`, clear `_response`, reset `_sent` et `_parser`.
- Destructeur : `delete _rh`, `close(_fd)`.

### `CGIReadHandler`, `CGIWriteHandler`, `TimerHandler`
**Vides.** Les fichiers `.cpp` existent mais ne contiennent rien. Aucune implémentation.

---

## TODO — points manquants ou à durcir

### 1. ❗ `CGIReadHandler` / `CGIWriteHandler` : tout à écrire
Voir [srcs/cgi/SUMMARY.md](../cgi/SUMMARY.md) pour le détail. En résumé :
- `CGIWriteHandler(int stdin_fd, const std::string& body)` : `handle_output()` envoie `body` par paquets via `write()` jusqu'à fin, puis `close(fd)` (EOF côté CGI), puis se retire de l'EventLoop et `delete this`.
- `CGIReadHandler(int stdout_fd, CGIHandler* parent)` : `handle_input()` lit dans un buffer accumulant, quand `read` retourne 0 → EOF → signale au parent + `delete this`.
- Les deux doivent référencer le `ClientHandler` (ou un callback) pour le réveiller en WRITE quand la sortie CGI est prête.

### 2. Architecture CGI async — décision à prendre
Aujourd'hui `_rh->buildResponse()` est synchrone. Pour CGI, la réponse ne peut pas être construite immédiatement.
→ Approche propre : `IRequestHandler::isReady()` retourne `false` tant que le CGI tourne. `ClientHandler::handle_output()` ne send que si `_rh->isReady()`. Sinon, se désinscrit de WRITE et attend que `CGIReadHandler` le réveille via `EventLoop::modify_handler(client, WRITE_EVENT)`.

---

## Priorisation

| Priorité  | Tâche                                          |
| --------- | ---------------------------------------------- |
| Critique  | 1 (CGI handlers vides)                         |
| Important | 2 (architecture CGI async)                     |

**Cœur du chantier restant.** `ServerHandler`/`ClientHandler` fonctionnent correctement pour GET/POST simple avec keep-alive et Connection: close. Les fichiers vides (`CGI*Handler`, `TimerHandler`) représentent le plus gros volume de code restant.
