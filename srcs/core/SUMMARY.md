# srcs/core/ — Résumé & TODO

## Rôle dans le projet

Ce dossier contient le **cœur événementiel** du serveur : la boucle unique qui surveille **tous** les file descriptors (sockets serveurs, sockets clients, pipes CGI à venir) via `epoll`, et dispatche chaque événement vers le bon handler.

C'est l'exigence la plus stricte du sujet :

> *"It must be non-blocking and use only **1** poll() (or equivalent) for all the I/O operations between the clients and the server (listen included). [...] You must never do a read or a write operation without going through poll() (or equivalent)."*

Si cette règle est cassée, c'est **0 au projet**.

### Pattern utilisé : Reactor (Schmidt, 1995)

Le code l'annonce explicitement dans les commentaires. Le découpage :

| Composant       | Rôle (Reactor)              | Responsabilité unique (SRP)                                       |
| --------------- | --------------------------- | ----------------------------------------------------------------- |
| `EventLoop`     | Initiation Dispatcher       | Possède l'unique `epfd`, surveille, dispatche, ne sait rien d'HTTP. |
| `IEventHandler` | Concrete Event Handler API  | Interface pure : `getFd()`, `handle_accept/input/output/close()`. |
| `HandlerEntry`  | Synchronous Event Demuxer item | Petit struct qui colle `(handler, type)` ; stocké dans `ev.data.ptr` pour qu'`epoll_wait` nous redonne directement le bon handler sans table de lookup par fd. |

**Pourquoi ce design est SOLID :**

- **SRP** : `EventLoop` ne fait que dispatcher. Les handlers font le métier.
- **OCP** : ajouter un nouveau type d'I/O (pipe CGI, signalfd, timerfd) = créer une classe qui implémente `IEventHandler` et l'enregistrer. `EventLoop` ne change jamais.
- **LSP** : tous les handlers sont interchangeables derrière `IEventHandler*` ; `EventLoop` ne fait jamais de `dynamic_cast`.
- **DIP** : `EventLoop` dépend de l'abstraction `IEventHandler`, pas de `ServerHandler` ou `ClientHandler` concrets.

---

## État actuel

### `IEventHandler` ([IEventHandler.hpp](../../includes/core/IEventHandler.hpp))
**Fait, propre.**
- `enum EventType` en puissances de 2 (octal), permet de combiner avec `|` et tester avec `&`.
- Méthodes `handle_*()` virtuelles avec corps vide → chaque handler n'override que ce qu'il utilise, pas de `switch` ni de cast côté EventLoop.
- Destructeur virtuel ✓.

### `EventLoop` ([EventLoop.hpp](../../includes/core/EventLoop.hpp) + [EventLoop.cpp](EventLoop.cpp))
**Largement fait.**
- Singleton avec `instance()` / `destroy()`, ctor/dtor privés.
- `epoll_create1(0)` dans le ctor.
- `register_handler` / `modify_handler` / `remove_handler` : crée/met à jour/supprime un `HandlerEntry*` dans `_table` (`map<IEventHandler*, HandlerEntry*>`) et fait l'appel `epoll_ctl` correspondant. Lookup O(log n).
- `to_epoll_flags()` traduit `EventType` → flags epoll.
- `handle_events()` : `epoll_wait` en boucle. Détecte `EPOLLHUP | EPOLLERR | EPOLLRDHUP` en premier → cleanup immédiat. Dispatche `EPOLLIN` → `handle_input()` et `EPOLLOUT` → `handle_output()` indépendamment (combinaison simultanée possible). Si une méthode retourne -1 → `remove_handler` + `delete h`.
- Signaux SIGINT/SIGTERM/SIGQUIT capturés via `g_stop` (volatile sig_atomic_t) → sortie propre.
- Destructeur libère tous les handlers et entries restants (utile au shutdown).

---

## TODO — points manquants ou à durcir

### 1. Dispatch incomplet : `handle_close()` jamais appelé
- `CLOSE_EVENT` → appeler `handle_close()` (non implémenté).
- `TIMEOUT_EVENT` (à brancher quand `TimerHandler` sera fait — voir handlers/SUMMARY.md).

### 2. Pas de propriété claire des handlers vs entries
- `register_handler` alloue `HandlerEntry*` que `EventLoop` possède (✓ documenté).
- Mais le `IEventHandler*` lui-même : qui le possède ? Aujourd'hui c'est ambigu. `ServerHandler` est créé dans `main.cpp` (`new ServerHandler(...)`) → leak si jamais on supprime un ServerHandler sans passer par `delete h` dans `handle_events`. Le destructeur d'`EventLoop` `delete` les handlers restants, donc OK au shutdown, mais en cours de run la convention "retourne -1 = je peux être détruit" est fragile.
→ Décision à acter : EventLoop est propriétaire de **tous** les `IEventHandler*` qu'on lui enregistre. Documenter et s'y tenir.

### 3. Pas de réplique pour `modify_handler` sur handler inconnu
[EventLoop.cpp:78-79](EventLoop.cpp#L78-L79) : si handler non trouvé → return silencieux. C'est défensif, mais ça masquera un bug si un handler oublie de s'enregistrer avant de demander une modif. Au moins un `std::cerr` en debug serait utile.

### 4. Pas de support `epoll_ctl` retour
Aucun des appels `epoll_ctl` ne vérifie le code de retour. Si `EPOLL_CTL_ADD` échoue (fd invalide, déjà dans epoll), on ne le sait jamais. → Au minimum, log + retour d'erreur depuis `register_handler` pour que l'appelant puisse rollback.

### 5. Timeout fixe à 500 ms
`epoll_wait(..., 500)`. Compromis raisonnable (réactivité à SIGINT) mais quand TimerHandler sera implémenté, le timeout devrait être calculé dynamiquement à partir du prochain timer à expirer (minimum entre 500ms et `next_timeout`).

### 6. `handle_close()` n'a pas de valeur de retour utilisée
L'interface définit `handle_close()` qui retourne `int`, mais EventLoop ne l'appelle pas. À aligner soit en supprimant `handle_close` de l'interface (s'il n'est utile à personne), soit en l'intégrant au dispatch.

### 7. Aucun mécanisme pour itérer/inspecter les handlers
Pour debug / stats / shutdown gracieux (laisser finir les réponses en cours), il sera utile d'avoir un getter ou un visiteur sur `_table`. À noter pour plus tard.

---

## Priorisation

| Priorité  | Tâche                                                        |
| --------- | ------------------------------------------------------------ |
| Important | 2 (ownership clarifié), 4 (vérifier `epoll_ctl`)             |
| Utile     | 3 (warning si introuvable), 5 (timeout dynamique), 6 (handle_close), 7 |
