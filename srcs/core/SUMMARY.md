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
- `register_handler` / `modify_handler` / `remove_handler` : crée/met à jour/supprime un `HandlerEntry*` et fait l'appel `epoll_ctl` correspondant.
- `to_epoll_flags()` traduit `EventType` → flags epoll.
- `handle_events()` : `epoll_wait(_epfd, events, 128, 500ms)` en boucle, lit `ev.data.ptr` pour récupérer le `HandlerEntry*`, appelle la bonne méthode selon le `type`. Si la méthode retourne -1 → `remove_handler` + `delete h`.
- Signaux SIGINT/SIGTERM/SIGQUIT capturés via `g_stop` (volatile sig_atomic_t) → sortie propre.
- Destructeur libère tous les handlers et entries restants (utile au shutdown).

---

## TODO — points manquants ou à durcir

### 1. Dispatch incomplet : `handle_close()` jamais appelé
[EventLoop.cpp:130-136](EventLoop.cpp#L130-L136) ne gère que `ACCEPT_EVENT`, `READ_EVENT`, `WRITE_EVENT`. Il manque :
- `CLOSE_EVENT` → appeler `handle_close()`.
- `TIMEOUT_EVENT` (à brancher quand `TimerHandler` sera fait — voir handlers/SUMMARY.md).
- Détection des erreurs epoll : `EPOLLHUP`, `EPOLLERR`, `EPOLLRDHUP` → typiquement → considérer le handler à supprimer (le client a fermé brutalement). Aujourd'hui, c'est seulement détecté indirectement quand `recv()` retourne 0/-1 dans `ClientHandler`.

### 2. Combinaison READ+WRITE simultanée impossible aujourd'hui
[EventLoop.cpp:131-136](EventLoop.cpp#L131-L136) compare `entry->type == READ_EVENT` (egalité stricte). Si jamais on enregistre un handler avec `READ_EVENT | WRITE_EVENT` (un client qui peut à la fois recevoir du body et écrire une réponse partielle), il ne tombera dans aucune branche → bug silencieux. Le sujet exige : *"poll() (or equivalent) must monitor both reading and writing simultaneously."*
→ Réécrire en `if (events[i].events & EPOLLIN) h->handle_input();` puis `if (events[i].events & EPOLLOUT) h->handle_output();`, en se basant sur **ce que epoll a retourné**, pas sur ce qu'on avait enregistré.
→ Bonus : ça permet aux handlers de surveiller READ et WRITE en même temps quand nécessaire (utile pour CGI duplex).

### 3. Pas de propriété claire des handlers vs entries
- `register_handler` alloue `HandlerEntry*` que `EventLoop` possède (✓ documenté).
- Mais le `IEventHandler*` lui-même : qui le possède ? Aujourd'hui c'est ambigu. `ServerHandler` est créé dans `main.cpp` (`new ServerHandler(...)`) → leak si jamais on supprime un ServerHandler sans passer par `delete h` dans `handle_events`. Le destructeur d'`EventLoop` `delete` les handlers restants, donc OK au shutdown, mais en cours de run la convention "retourne -1 = je peux être détruit" est fragile.
→ Décision à acter : EventLoop est propriétaire de **tous** les `IEventHandler*` qu'on lui enregistre. Documenter et s'y tenir.

### 4. Pas de réplique pour `modify_handler` sur handler inconnu
[EventLoop.cpp:81-82](EventLoop.cpp#L81-L82) : si `entry` non trouvé → return silencieux. C'est défensif, mais ça masquera un bug si un handler oublie de s'enregistrer avant de demander une modif. Au moins un `std::cerr` en debug serait utile.

### 5. `_table` est un `vector<HandlerEntry*>` parcouru linéairement
`modify_handler` et `remove_handler` font une recherche O(n) sur `_table`. Avec quelques dizaines de connexions OK, mais sous stress test (cf. exigence du sujet : *"Stress test your server to ensure it remains available at all times."*) ça devient quadratique. Une `map<IEventHandler*, HandlerEntry*>` règle le souci sans coût mémoire significatif.

### 6. Pas de support `epoll_ctl` retour
Aucun des appels `epoll_ctl` ne vérifie le code de retour. Si `EPOLL_CTL_ADD` échoue (fd invalide, déjà dans epoll), on ne le sait jamais. → Au minimum, log + retour d'erreur depuis `register_handler` pour que l'appelant puisse rollback.

### 7. Timeout fixe à 500 ms
[EventLoop.cpp:121](EventLoop.cpp#L121) : `epoll_wait(..., 500)`. Compromis raisonnable (réactivité à SIGINT) mais quand TimerHandler sera implémenté, le timeout devrait être calculé dynamiquement à partir du prochain timer à expirer (minimum entre 500ms et `next_timeout`). Sinon on perd jusqu'à 500ms de précision sur les timeouts client/CGI.

### 8. `handle_close()` n'a pas de valeur de retour utilisée
L'interface définit `handle_close()` qui retourne `int`, mais EventLoop ne l'appelle pas. À aligner soit en supprimant `handle_close` de l'interface (s'il n'est utile à personne), soit en l'intégrant au dispatch.

### 9. Aucun mécanisme pour itérer/inspecter les handlers
Pas de critique fondamentale, mais pour debug / panneau de stats / gestion des shutdowns gracieux (laisser finir les réponses en cours avant d'arrêter), il sera utile d'avoir un getter `const std::vector<HandlerEntry*>&` ou un visiteur. À noter pour plus tard.

---

## Priorisation

| Priorité  | Tâche                                                                     |
| --------- | ------------------------------------------------------------------------- |
| Critique  | 2 (READ+WRITE simultané — exigence directe du sujet), 1 (EPOLLHUP/ERR)    |
| Important | 3 (ownership clarifié), 6 (vérifier `epoll_ctl`), 7 (timeout dynamique)   |
| Utile     | 5 (map au lieu de vector), 4 (warning si introuvable), 8 (handle_close), 9 |

**Le code core est solide structurellement** (pattern Reactor bien appliqué, doc claire). Les manques sont du durcissement, pas du redesign. Le point 2 est le seul qui touche directement à une exigence formelle du sujet.
