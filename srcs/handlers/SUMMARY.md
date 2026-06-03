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
- `handle_accept` : `accept()`, `new ClientHandler(client_fd, _server)` (le ClientHandler s'enregistre lui-même dans son ctor).
- `printServer()` : affiche joliment la config résolue (debug).
- RAII : try/catch dans le ctor referme le fd en cas d'échec ; destructeur `close(_fd)`.

### `ClientHandler` ([ClientHandler.cpp](ClientHandler.cpp))
**Partiellement fait — fonctionne pour un cas simple.**
- Constructeur : `register_handler(this, READ_EVENT)` immédiatement après accept.
- `handle_input` : `recv()` 4096 octets, push dans `HttpParser`, si `COMPLETE` → `Router::route(req, _server)` → switch en `WRITE_EVENT`, si `ERROR` → retour -1.
- `handle_output` : `send()` la réponse, gère le send partiel via `_sent`, quand fini → reset parser, switch en `READ_EVENT` (keep-alive).
- Destructeur : `close(_fd)`.

### `CGIReadHandler`, `CGIWriteHandler`, `TimerHandler`
**Vides.** Les fichiers `.hpp` et `.cpp` existent mais ne contiennent rien. Aucune implémentation.

---

## TODO — points manquants ou à durcir

### 1. ❗ `ClientHandler` ne nettoie pas le `IRequestHandler*` qu'il alloue
[ClientHandler.cpp:47](ClientHandler.cpp#L47) : `_rh = Router::route(...)` alloue un nouvel handler à chaque requête, mais nulle part on ne `delete _rh`. À chaque requête keep-alive : **leak**. Et le destructeur ne le delete pas non plus.
→ `delete _rh` quand la réponse a été envoyée (fin de `handle_output`), avec `_rh = NULL`. Idem dans le destructeur si non-NULL.

### 2. ❗ `ClientHandler::handle_output` ne gère pas `send()` qui retournerait EAGAIN
`send()` non bloquant peut retourner `-1` avec `errno = EAGAIN`. Le code retourne `-1` → EventLoop supprime le handler → connexion tuée alors qu'il fallait juste réessayer. Or vérifier `errno` après read/write est **interdit** par le sujet ! → La solution propre : ne pas considérer `n <= 0` comme une erreur fatale ; revenir sur `epoll_wait` (qui ne re-signalera `EPOLLOUT` que quand prêt). Voir aussi le point 2 de core/SUMMARY (READ+WRITE simultané) — c'est la même famille de problèmes.

### 3. ❗ Pas de timeout (point critique du sujet)
*"A request to your server should never hang indefinitely."* Aujourd'hui, un client qui ouvre une connexion et n'envoie rien reste là pour toujours.

→ Implémenter `TimerHandler`. Deux options :

- **Option A (simple, sans timerfd)** : `EventLoop::handle_events()` parcourt `_table` après chaque `epoll_wait` et vérifie un `time_t last_activity` exposé par chaque handler. Si > 30s → `remove + delete`.
- **Option B (propre, avec timerfd)** : `TimerHandler` créé par `accept`, possède un `timerfd_create`, surveillé par epoll. Au tick : tue le client. Réarmé à chaque `handle_input/output` du client.

Le commentaire dans [ideas.txt](../../ideas.txt) suggère l'option B : *"quand on accept un client on creer un objet clienthandler, qui lui creera un timehandler [...] le timer est remis a 30 ou 15s a chaque action du client (recv/send) et le cgi aura la meme logique"*. Préférer cette piste pour cohérence.

⚠️ `timerfd_create` n'est pas dans la liste des fonctions autorisées du sujet. → fallback : option A (poll des timeouts dans `handle_events`).

### 4. ❗ `ClientHandler` reset le parser mais garde `_buffer` (HttpParser bug lié)
[ClientHandler.cpp:48](ClientHandler.cpp#L48) appelle `_parser.reset()` après avoir récupéré la requête. Mais [HttpParser.cpp:103-110](../http/HttpParser.cpp#L103-L110) ne clear pas `_buffer`/`_body`/`_header` (lignes commentées). → Sur keep-alive, la deuxième requête se mélange avec le résidu de la première. À débloquer.

### 5. `ServerHandler` : `setsockopt` non vérifié
[ServerHandler.cpp:83](ServerHandler.cpp#L83) : `setsockopt(... SO_REUSEADDR ...)` ignore le retour. Si ça échoue, on aura "Address already in use" au prochain bind. Petit point mais qui sauvera 5 min de debug à la défense.

### 6. `ServerHandler` : `fcntl` non vérifié
[ServerHandler.cpp:84](ServerHandler.cpp#L84) : pareil. Même remarque.

### 7. `ClientHandler` : pas de `Connection: close` respecté
Si la requête contient `Connection: close` (HTTP/1.1) ou n'a pas `Connection: keep-alive` (HTTP/1.0), il faut fermer après envoi de la réponse au lieu de repasser en READ_EVENT. Aujourd'hui [ClientHandler.cpp:64-68](ClientHandler.cpp#L64-L68) repasse systématiquement en lecture. → Récupérer le header Connection dans HttpRequest et brancher la logique.

### 8. `CGIReadHandler` / `CGIWriteHandler` : tout à écrire
Voir [srcs/cgi/SUMMARY.md](../cgi/SUMMARY.md) pour le détail. En résumé :
- `CGIWriteHandler(int stdin_fd, const std::string& body)` : `handle_output()` envoie `body` par paquets via `write()` jusqu'à fin, puis `close(fd)` (EOF côté CGI), puis se retire de l'EventLoop et `delete this`.
- `CGIReadHandler(int stdout_fd, CGIHandler* parent)` : `handle_input()` lit dans un buffer accumulant, quand `read` retourne 0 → EOF → signale au parent + `delete this`.
- Les deux doivent référencer le `ClientHandler` (ou un callback) pour le réveiller en WRITE quand la sortie CGI est prête.

### 9. `TimerHandler` : tout à écrire — voir 3 ci-dessus.

### 10. Couplage temporaire `ClientHandler` ↔ `IRequestHandler`
Aujourd'hui `_rh` est stocké comme attribut. Pour CGI ce sera différent : le `CGIHandler` ne peut pas construire sa réponse en `handle_output()` direct, il faut attendre le CGI. → Soit `IRequestHandler::buildResponse()` devient non bloquant (retourne `""` tant que pas prêt), soit le ClientHandler attend un signal "ready". Décision à prendre tôt pour éviter de refactorer plus tard.

Une approche propre : `IRequestHandler::isReady()` qui retourne `false` tant que la réponse n'est pas prête (CGI en cours). `ClientHandler::handle_output()` ne send que si `_rh->isReady()`. Sinon, il se désinscrit de WRITE et attend que le CGIReadHandler le réveille via `EventLoop::modify_handler(client, WRITE_EVENT)`.

### 11. `ClientHandler` : pas de `peer_addr` stocké
`accept()` dans `ServerHandler::handle_accept()` ignore le `client_addr` rempli. Pour les logs (utile à la défense) et la variable CGI `REMOTE_ADDR`, il faut la passer au `ClientHandler` et la conserver.

---

## Priorisation

| Priorité  | Tâche                                                                              |
| --------- | ---------------------------------------------------------------------------------- |
| Critique  | 1 (leak `_rh`), 2 (send EAGAIN), 3 (timeout), 4 (parser reset incomplet), 8 (CGI handlers vides) |
| Important | 7 (Connection: close), 9 (TimerHandler), 10 (architecture CGI async)               |
| Utile     | 5/6 (vérifs setsockopt/fcntl), 11 (REMOTE_ADDR)                                    |

**Cœur du chantier restant.** Les `ServerHandler`/`ClientHandler` actuels fonctionnent en démo pour GET simple mais ne résistent ni au stress test ni aux exigences de timeout/CGI du sujet. Les fichiers vides (`CGI*Handler`, `TimerHandler`) représentent le plus gros volume de code à écrire dans tout le projet.
