# srcs/cgi/ — Résumé & TODO

## Rôle dans le projet

Ce dossier doit contenir **toute la logique d'exécution CGI** : c'est ici qu'on fait le pont entre une requête HTTP qui matche `cgi_pass .ext /chemin/interpreteur` (config) et un processus enfant qui exécute le script et renvoie sa sortie.

Le sujet impose explicitement :

- Support d'au moins un CGI (php-CGI, python, etc.).
- Les variables d'environnement CGI doivent être correctement passées (`REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `SCRIPT_NAME`, `PATH_INFO`, `SERVER_PROTOCOL`, etc. — voir RFC 3875).
- Le body de la requête (un-chunké au préalable par le serveur) est passé au CGI via stdin.
- Si le CGI ne renvoie pas de `Content-Length`, le EOF du pipe marque la fin du body.
- Le CGI doit s'exécuter dans le bon répertoire courant (cwd = dossier du script, pour les chemins relatifs).
- L'exécution doit rester **non bloquante** : pas de `read()`/`write()` direct sur les pipes — tout passe par l'unique `epoll` central.

### Découpage SOLID attendu

L'idée : `CGIHandler` (dans `srcs/http/`) implémente `IRequestHandler` mais **ne fait pas le travail lui-même**. Il délègue à ce dossier :

| Classe                | Responsabilité unique (SRP)                                                                |
| --------------------- | ------------------------------------------------------------------------------------------ |
| `CGIProcess` (cgi/)   | Fork + execve + pipes + setup environnement. Ne sait rien d'HTTP, juste : lance un binaire avec un env et un body sur stdin, et expose deux fd (write stdin, read stdout). |
| `CGIEnv` (cgi/)       | Construit le `char**` envp à partir d'une `HttpRequest` + `Location`. Pure transformation, testable isolément. |
| `CGIWriteHandler` (handlers/) | `IEventHandler` qui pousse le body de la requête vers stdin du CGI au rythme de `epoll`. |
| `CGIReadHandler` (handlers/)  | `IEventHandler` qui lit la stdout du CGI jusqu'à EOF, accumule, puis demande à `CGIHandler` de construire la réponse. |
| `CGIHandler` (http/)  | `IRequestHandler` : orchestre, expose `buildResponse()` qui parse la sortie CGI (headers CGI → headers HTTP, status line) et renvoie la réponse finale. |

Cette séparation respecte **SRP** (chaque classe a une seule raison de changer), **OCP** (ajouter un autre type CGI = nouvelle config, pas de modif des classes), **DIP** (`ClientHandler` dépend de l'abstraction `IRequestHandler`, jamais du concret CGI).

---

## État actuel

**Le dossier est vide.** Seul un `.gitkeep` est présent. Aucune ligne de code CGI n'a été écrite.

Ce qui existe ailleurs et qui se rapporte au CGI :

- `includes/handlers/CGIReadHandler.hpp` et `CGIWriteHandler.hpp` → fichiers vides (0 ligne).
- `srcs/handlers/CGIReadHandler.cpp` et `CGIWriteHandler.cpp` → fichiers vides (0 ligne).
- `includes/http/CGIHandler.hpp` + `srcs/http/CGIHandler.cpp` → stub qui retourne `"CGI handler response"` en dur. Pas de fork, pas d'exec.
- La config parse bien `cgi_pass .ext /chemin/interpreteur` et le `Validator` vérifie que l'extension commence par `.` et que l'interpréteur est un chemin absolu.
- Le `Router` ne route encore **rien** vers CGI : le code `if (isCgi())` est commenté dans [Router.cpp:31](Router.cpp#L31), tout retombe sur `StaticHandler`.

Bref : **0% fait, mais les emplacements et l'architecture sont prêts à être remplis.**

---

## TODO — gros points manquants

### 1. Détection CGI dans le Router
Avant tout, le `Router` doit savoir reconnaître une requête CGI :
- Extraire l'extension du fichier ciblé par l'URI (après application du `root` de la `Location`).
- Vérifier que `Location::cgi_pass` contient cette extension.
- Si oui → instancier `CGIHandler(req, *loc)` au lieu de `StaticHandler`.
- Penser au cas `PATH_INFO` : `/script.py/extra/path` doit séparer le script (`/script.py`) du PATH_INFO (`/extra/path`).

### 2. Construire l'environnement CGI (`CGIEnv`)
Variables minimales exigées par la RFC 3875 + ce qu'attendent php-cgi / python :
- `GATEWAY_INTERFACE=CGI/1.1`
- `SERVER_PROTOCOL=HTTP/1.1`
- `SERVER_SOFTWARE=webserv/1.0`
- `REQUEST_METHOD` (GET/POST/DELETE)
- `QUERY_STRING` (partie après `?` dans l'URI, sans le `?`)
- `CONTENT_LENGTH` (body size, en string)
- `CONTENT_TYPE` (header `Content-Type`)
- `SCRIPT_NAME` / `SCRIPT_FILENAME` (chemin du script)
- `PATH_INFO` / `PATH_TRANSLATED`
- `SERVER_NAME`, `SERVER_PORT`, `REMOTE_ADDR`
- `REDIRECT_STATUS=200` (obligatoire pour php-cgi sinon il refuse)
- Tous les headers HTTP préfixés `HTTP_` (ex: `User-Agent` → `HTTP_USER_AGENT`)

Cette classe doit produire un `char**` envp prêt pour `execve()` et libérer proprement la mémoire (RAII : destructeur qui delete chaque char* + le tableau).

### 3. Fork + pipes + execve (`CGIProcess`)
- Créer deux `pipe()` : un pour stdin du CGI (parent écrit, enfant lit), un pour stdout (enfant écrit, parent lit).
- Marquer les deux extrémités côté parent en `O_NONBLOCK` + `FD_CLOEXEC` (autorisé par le sujet).
- `fork()`. Côté enfant :
  - `dup2()` les bonnes extrémités sur 0 et 1.
  - Fermer tous les autres fd hérités (sinon fuites de descripteurs).
  - `chdir()` vers le dossier du script (exigence du sujet : cwd correct pour les chemins relatifs).
  - `execve(interpreter, argv, envp)` avec `argv = { interpreter, script_path, NULL }`.
  - Si execve échoue : `exit(1)` (pas de cout, l'enfant n'a plus de contexte propre).
- Côté parent : fermer les extrémités inutiles, conserver le `pid` pour `waitpid` plus tard.

### 4. Brancher les pipes sur l'EventLoop
- Créer un `CGIWriteHandler(stdin_fd, &body, &state)` qui implémente `IEventHandler::handle_output()`. Il écrit progressivement le body de la requête, et quand tout est envoyé, il `close()` le fd (EOF côté CGI) et se retire de l'EventLoop.
- Créer un `CGIReadHandler(stdout_fd, &output, &state)` qui implémente `IEventHandler::handle_input()`. Il accumule la sortie. Quand `read()` retourne 0 → EOF → le CGI a fini → signaler au `CGIHandler` que la sortie est prête, puis basculer le `ClientHandler` associé en `WRITE_EVENT`.
- Les deux handlers doivent garder un pointeur vers le `ClientHandler` ou un callback pour signaler la fin.

### 5. Un-chunking du body avant transmission
Le sujet précise : *"for chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the body."* Donc le `HttpParser` doit déjà avoir reconstruit le body complet (sans les marqueurs `\r\n<size>\r\n`) avant qu'il soit poussé au CGI. → C'est une dépendance côté `HttpParser` (qui n'implémente pas encore le chunked transfer-encoding).

### 6. Parsing de la sortie CGI
Le CGI renvoie d'abord des **headers CGI** (`Content-Type:`, `Status:`, `Location:`, ...) puis `\r\n\r\n` puis le body. `CGIHandler::buildResponse()` doit :
- Trouver le séparateur `\r\n\r\n`.
- Mapper `Status: 302 Found` → première ligne `HTTP/1.1 302 Found`. Par défaut : `200 OK`.
- Recopier les headers CGI dans la réponse HTTP.
- Ajouter `Content-Length` calculé à partir du body si le CGI ne l'a pas fourni, OU utiliser le mode "EOF" mentionné dans le sujet (Connection: close + pas de Content-Length, fermer la socket à la fin).
- Concaténer le body.

### 7. Timeout CGI
Un CGI qui boucle infiniment ne doit pas hanger le serveur. Le `TimerHandler` (déjà esquissé dans handlers/) devrait être réutilisé : armer un timer (ex: 30s) au démarrage du CGI, le désarmer à la fin. Si expiré → `kill(pid, SIGKILL)`, libérer les handlers, renvoyer `504 Gateway Timeout` au client.

### 8. `waitpid` et zombies
Quand le CGI a fini (EOF lecture stdout), il faut faire `waitpid(pid, &status, WNOHANG)` pour récolter le zombie. Si `WIFEXITED` && `WEXITSTATUS != 0` → log + considérer en erreur (mais on a déjà la sortie partielle, c'est un choix de design : NGINX renvoie quand même ce qu'il a reçu).

### 9. Gestion d'erreurs robuste
- `pipe()` échoue → 500.
- `fork()` échoue → 500.
- `execve()` n'est jamais atteint normalement (sinon l'enfant exit 1) → le parent voit `WEXITSTATUS == 1` et renvoie 500.
- `SIGPIPE` lors d'un write sur un CGI qui a déjà fermé son stdin → ignorer (`signal(SIGPIPE, SIG_IGN)` au démarrage du serveur, ou `MSG_NOSIGNAL` sur `send`).
- Limiter le nombre de CGI concurrents (sinon fork bomb sur stress test) → optionnel mais utile.

### 10. Tests à prévoir
- Script python qui lit stdin et renvoie le hash → vérifier que POST passe bien.
- Script qui boucle → vérifier le timeout.
- Script qui sort sans Content-Length → vérifier le mode EOF.
- Script qui renvoie `Status: 500` → vérifier le mapping.
- `php-cgi` avec un `.php` minimal → couvre le cas multi-interpréteurs (bonus du sujet).

---

## Priorisation

| Priorité  | Tâche                                                              |
| --------- | ------------------------------------------------------------------ |
| Critique  | 1 (détection routeur), 2 (env), 3 (fork/exec), 4 (pipes/eventloop) |
| Important | 6 (parsing sortie), 7 (timeout), 8 (waitpid)                       |
| Utile     | 5 (un-chunking — dépend du HttpParser), 9 (SIGPIPE), 10 (tests)    |

Tant que 1–4 ne sont pas faits, le serveur ne peut pas exécuter un seul script. C'est le bloc à attaquer en premier.
