# CORRECTIONS_BUGS.md — Bugs corrigés, un par un

> Objectif du passage : **corriger les bugs sans changer la logique** de chaque fonction.
> Chaque entrée décrit le symptôme, la cause exacte, le fichier/ligne, et la correction minimale appliquée.
> Toutes les corrections compilent avec `-Wall -Wextra -Werror -std=c++98` et ont été testées en exécution (voir la fin du document).

---

## 1. CGI totalement non fonctionnel — `chdir` + chemin relatif dédoublé
**Fichier :** `srcs/cgi/CGIProcess.cpp` (`CGIFork`)
**Symptôme :** toute requête CGI renvoyait `500 Internal Server Error`.
**Cause :** l'enfant faisait `chdir(dir)` (dossier du script) **puis** passait à `execve` le chemin **relatif complet** (`www/cgi-bin/hello.py`). Après le `chdir` dans `www/cgi-bin`, l'interpréteur cherchait `www/cgi-bin/www/cgi-bin/hello.py` → fichier introuvable → l'interpréteur sortait avec un code ≠ 0 → 500.
**Correction :** après le `chdir`, on passe désormais le **basename** du script (`hello.py`) comme argument. On vérifie aussi le retour de `chdir` (exit 1 si échec). La logique « se placer dans le dossier du script » est conservée, seul l'argument transmis est corrigé.

## 2. Sortie CGI collée telle quelle dans la réponse HTTP
**Fichier :** `srcs/handlers/CGIReadHandler.cpp`
**Symptôme :** les en-têtes du script (`Content-Type: ...`) s'affichaient **dans le corps** de la page, et le `Content-Type` du script n'était jamais repris dans la réponse HTTP.
**Cause :** la sortie brute du CGI était injectée directement comme body via `StaticHandler(loc, 200, _out)`. Or la sortie d'un CGI est un **bloc d'en-têtes CGI** (`Status:`, `Content-Type:`) + ligne vide + body (RFC 3875).
**Correction :** ajout de `buildCgiResponse()` qui sépare les en-têtes CGI du body, lit `Status:` (code HTTP) et `Content-Type:`, puis construit la réponse. Gère `\r\n\r\n` **et** `\n\n` (scripts qui n'émettent que des `\n`).

## 3. CGI bloquant : `waitpid(pid, &status, 0)` gelait tout le serveur
**Fichier :** `srcs/handlers/CGIReadHandler.cpp`
**Symptôme :** un script qui ferme son stdout mais continue de tourner (boucle infinie) figeait **l'ensemble** du serveur.
**Cause :** `waitpid(..., 0)` est bloquant.
**Correction :** `reapChild()` utilise `waitpid(..., WNOHANG)` ; si l'enfant tourne encore, on le `kill(SIGKILL)` puis on le récupère. Le serveur reste réactif (vérifié : requêtes servies pendant qu'un CGI boucle).

## 4. Aucun timeout CGI → requête en attente indéfinie
**Fichiers :** `includes/handlers/CGIReadHandler.hpp`, `srcs/handlers/CGIReadHandler.cpp`
**Symptôme :** un CGI qui ne répond jamais laissait le client pendu à vie (interdit par le sujet).
**Cause :** `CGIReadHandler` ne participait pas au mécanisme de timeout de l'EventLoop (`getLastActivity()` renvoyait 0).
**Correction :** ajout de `_lastActivity` + override `getLastActivity()`. Le CGI est désormais soumis au même timeout que les clients ; à l'expiration le destructeur tue l'enfant et renvoie `504 Gateway Timeout`.

## 5. Use-after-free : client et CGI se référencent mutuellement
**Fichiers :** `includes/core/IResponseSink.hpp`, `ClientHandler.{hpp,cpp}`, `CGIReadHandler.{hpp,cpp}`, `CGIHandler.cpp`
**Symptôme :** si le client se déconnecte pendant qu'un CGI tourne (ou l'inverse), l'objet survivant appelait une méthode sur un objet déjà détruit → crash potentiel.
**Cause :** `CGIReadHandler` gardait un `IResponseSink*` (le `ClientHandler`) sans être prévenu de sa destruction.
**Correction :** liaison bidirectionnelle. `onCgiStart()` donne au client un pointeur vers son `CGIReadHandler` ; `~ClientHandler` appelle `detachSink()` pour couper le lien ; côté CGI, `_sink` est remis à NULL après usage. Aucun des deux ne touche plus l'autre après destruction.

## 6. Zombies et fuites de fd CGI
**Fichiers :** `srcs/handlers/CGIReadHandler.cpp` (destructeur), `srcs/cgi/CGIProcess.cpp`
**Symptôme :** `~CGIProcess` était **vide** → `_write_fd`/`_read_fd` jamais fermés ; pas de `waitpid` fiable → zombies.
**Correction :** le `CGIReadHandler` ferme son fd de lecture et reap l'enfant dans son destructeur (`_reaped` évite un double-reap sur un pid recyclé). `CGIWriteHandler` ferme déjà son fd. Nettoyage complet en cas d'échec de `fcntl` dans `CGIFork` (fds fermés + enfant tué).

## 7. `FD_CLOEXEC` manquant → l'enfant CGI héritait des sockets
**Fichiers :** `srcs/core/EventLoop.cpp`, `srcs/handlers/ServerHandler.cpp`, `srcs/cgi/CGIProcess.cpp`
**Symptôme :** l'enfant CGI héritait du socket d'écoute, des sockets clients, du fd epoll et des pipes des autres CGI. Conséquence grave : un fd d'écriture de pipe hérité empêche le lecteur d'en face de voir l'EOF (blocage).
**Correction :** `FD_CLOEXEC` posé sur le fd epoll, le socket d'écoute, chaque socket client accepté, et les deux extrémités de pipe côté parent.

## 8. `(*end)++` — écriture en mémoire const + boucle infinie (chunked)
**Fichier :** `srcs/http/HttpParser.cpp` (`readChunked`)
**Symptôme :** DoS : toute requête *chunked* avec un espace après la taille de chunk provoquait une boucle infinie.
**Cause :** `(*end)++` incrémentait le **caractère pointé** (dans le buffer const de `c_str()`, comportement indéfini) au lieu d'avancer le **pointeur**. `end` n'avançait jamais.
**Correction :** `end++`. On tolère aussi `;` (chunk-extension légale, RFC 7230).

## 9. Terminateur *chunked* jamais consommé → keep-alive corrompu
**Fichier :** `srcs/http/HttpParser.cpp` (`readChunked`)
**Symptôme :** en keep-alive après une requête chunked, la requête suivante était corrompue.
**Cause :** le `0\r\n\r\n` final n'était pas retiré de `_buffer`.
**Correction :** on attend le terminateur complet puis on l'efface de `_buffer` (sauf si déjà en état d'erreur).

## 10. `HttpParser::reset()` — reliquat pipeliné jamais re-parsé
**Fichiers :** `srcs/http/HttpParser.cpp`, `srcs/handlers/ClientHandler.cpp`
**Symptôme :** deux requêtes envoyées d'un coup (pipelining) sur une connexion keep-alive → **une seule** servie, l'autre perdue jusqu'au timeout.
**Cause :** après envoi de la réponse, aucun octet n'arrive pour redéclencher `handle_input`, donc le reliquat déjà présent dans `_buffer` n'était jamais traité. (`reset()` conservait déjà `_buffer`, ce qui est correct.)
**Correction :** après un `_reset()` en keep-alive, `handle_output` relance `runParsing("")` sur le buffer restant et traite immédiatement une éventuelle requête déjà complète. **Vérifié : 2 réponses `200` sur une seule socket pipelinée.**

## 11. Consommation du body au-delà de `Content-Length`
**Fichier :** `srcs/http/HttpParser.cpp` (bloc `R_BODY`)
**Symptôme :** en pipelining, le body « avalait » le début de la requête suivante ; et un body plus long que `Content-Length` renvoyait `400` au lieu d'ignorer le surplus.
**Cause :** `_body += _buffer` copiait **tout** le buffer sans se limiter à `Content-Length`.
**Correction :** on ne consomme que `Content-Length - déjà_reçu` octets, le reste demeure dans `_buffer` pour la requête suivante.

## 12. `Connection: close` codé en dur → keep-alive impossible
**Fichiers :** `srcs/http/ARequestHandler.cpp`, `includes/http/ARequestHandler.hpp`, `ClientHandler.cpp`
**Symptôme :** la réponse annonçait toujours `Connection: close`, contredisant la logique keep-alive du `ClientHandler`.
**Correction :** `buildResponse` émet `keep-alive` ou `close` selon `_keepAlive`, transmis par `ClientHandler::setKeepAlive()`. Les réponses CGI restent en `close` (elles ne connaissent pas le keep-alive du client).

## 13. `Transfer-Encoding: chunked` détecté sur n'importe quel header
**Fichier :** `srcs/http/HttpParser.cpp` (`headerParser`)
**Symptôme :** un header quelconque valant `chunked` (ex. `X-Foo: chunked`) activait le mode chunked.
**Cause :** le test portait sur la **valeur** (`it->second == "chunked"`), pas sur la clé.
**Correction :** test sur la **clé** `Transfer-Encoding` contenant `chunked`.

## 14. Ordre des vérifications du Router incohérent + code mort
**Fichier :** `srcs/http/Router.cpp` (`route`)
**Symptômes/causes :**
- `methodAllowed` était appelé **deux fois** (dont un mort).
- `501` (non implémenté) et `405` (non autorisé) étaient testés **après** l'accès disque → un vrai `404`/`403` masquait le bon code.
- La location était cherchée avec la **query string** (`/upload?x=1` ne matchait jamais `/upload`).
- La redirection (`return`) n'était appliquée qu'ensuite → une URL sans fichier réel renvoyait `404` au lieu de rediriger.
**Correction :** ordre remis à `501 → 405 → return → résolution chemin → 404 → 403 → CGI → static`. Matching de location fait sur l'URI **sans** query string. Doublon supprimé.

## 15. `resolvePath` : l'index n'était jamais servi
**Fichier :** `srcs/http/Router.cpp` (`resolvePath`)
**Symptôme :** `GET /` renvoyait une redirection vers un chemin filesystem au lieu de servir `index.html`.
**Cause :** `normalizePath` **supprime le slash final**, donc `path[len-1] == '/'` était toujours faux → l'index n'était jamais ajouté.
**Correction :** on teste réellement `isDir(path)` avant d'ajouter l'index. Si l'index est absent et `autoindex on`, on laisse le listing s'appliquer. **Vérifié : `GET /` sert bien l'index.**

## 16. Path traversal (`normalizePath` : test de préfixe insuffisant)
**Fichier :** `srcs/utils/utils.cpp` (`normalizePath`)
**Symptômes/causes :**
- `result.find(root) != 0` n'était qu'un test de **sous-chaîne** : `/var/www-evil` passait pour `/var/www`.
- Un `root` **relatif** (`www`, `./www`) ne matchait jamais le chemin canonisé → `403` systématique.
**Correction :** extraction de `canonPath()` (résout `.`/`..`, gère relatif/absolu). Comparaison sur **frontière de segment** (le caractère suivant `root` doit être `/` ou rien). Empilement de `..` en tête corrigé pour ne pas repop un `..`.
**Vérifié : `GET /../../../etc/passwd` → `403`, plus de fuite.**

## 17. Upload : traversal via `filename` + erreurs d'écriture silencieuses
**Fichier :** `srcs/http/MultipartHandler.cpp`
**Symptômes/causes :**
- Un `filename` client contenant `../../x` permettait d'écrire **hors** du dossier d'upload.
- L'échec d'ouverture du fichier n'était pas détecté (toujours `200`/`201`).
**Correction :** on ne garde que le **basename** du `filename`. Vérification de `file.is_open()` → `500` en cas d'échec. **Vérifié : `filename=../../evil.txt` reste dans `www/upload/`, ne s'échappe pas.**

## 18. POST : test d'existence après ouverture → jamais de `201`
**Fichier :** `srcs/http/StaticHandler.cpp` (`handlePost`)
**Symptôme :** un POST créant un nouveau fichier renvoyait `200` au lieu de `201 Created`.
**Cause :** `fileFound(_path)` était appelé **après** `std::ofstream file(_path)` qui a déjà créé le fichier.
**Correction :** on teste l'existence **avant** d'ouvrir le flux. Ajout d'un `403` si le fichier ne peut être ouvert (dossier absent/non inscriptible).

## 19. GET : fichier illisible renvoyait `200` avec body vide
**Fichier :** `srcs/http/StaticHandler.cpp` (`handleGet`)
**Correction :** si le fichier existe (vérifié par le Router) mais que l'`ifstream` ne s'ouvre pas (droits), on renvoie `403` au lieu d'un `200` vide.

## 20. `error_page` de location jamais utilisées
**Fichier :** `srcs/http/StaticHandler.cpp` (constructeur principal)
**Symptôme :** les pages d'erreur configurées dans une location n'étaient jamais servies.
**Cause :** la recherche `loc.error_page.find(_ncode)` se faisait **en début de constructeur**, avec `_ncode` encore à `200`.
**Correction :** la recherche est déplacée **à la fin**, sur le code réellement produit.

## 21. Directory listing : chemin recalculé + HTML tronqué
**Fichier :** `srcs/http/StaticHandler.cpp` (`throwList`, `headerListe`)
**Symptômes/causes :**
- Le chemin était recalculé via `root + uri` (ré-injectant la query string → `opendir` échouait).
- Les balises fermantes (`</ul></body></html>`) étaient ajoutées à la variable **locale** `html` **après** sa copie dans `_body` → jamais envoyées.
- `autoindex` n'était pas vérifié à cet endroit.
**Correction :** réutilisation de `_path` (déjà résolu/normalisé), balises fermantes ajoutées à `_body`, vérification de `autoindex` (sinon `403`), gestion de l'échec `opendir` (`403`).

## 22. Redirection `return` : header `Location` sur codes non-3xx + HTML malformé
**Fichier :** `srcs/http/ARequestHandler.cpp` (`handleReturn`)
**Symptômes/causes :**
- `Location:` était émis même pour un `return 500 texte` (non-sens : `Location` ne vaut que pour 3xx).
- Balises HTML inversées : `<h1>...</html></body></h1>`.
**Correction :** `Location` émis uniquement pour 3xx ; pour les autres codes le second argument sert de message dans le body. HTML corrigé.

## 23. `getErrorPage` : page configurée mais introuvable → body vide
**Fichier :** `srcs/http/ARequestHandler.cpp` (`getErrorPage`)
**Correction :** si le fichier de page d'erreur ne s'ouvre pas, on retombe sur la page d'erreur **par défaut** générée au lieu d'un body vide.

## 24. `StaticHandler(loc, code, body)` : pointeurs membres non initialisés
**Fichier :** `srcs/http/StaticHandler.cpp`
**Symptôme :** UB latent — `_req` et `_loc` laissés indéterminés dans ce constructeur (utilisé par le CGI).
**Correction :** `: _req(NULL), _loc(&loc)` dans la liste d'initialisation.

## 25. `EventLoop` : le serveur pouvait crasher / boucler / timeouts gelés
**Fichier :** `srcs/core/EventLoop.cpp` (`handle_events`)
**Symptômes/causes :**
- Une exception lancée par un handler remontait et **arrêtait le serveur** (interdit par le sujet).
- `if (n <= 0) continue;` **sautait `checkTimeOut()`** : quand le serveur était inactif, les timeouts ne tombaient jamais.
- Un fd signalé `EPOLLERR`/`EPOLLHUP` **seul** (ni IN ni OUT) n'était jamais fermé → epoll le re-signalait en boucle (busy loop 100 % CPU).
**Correction :** chaque dispatch de handler est enveloppé dans un `try/catch` (le handler fautif est retiré, le serveur survit) ; `checkTimeOut()` est appelé à **chaque** tour ; un `EPOLLERR/HUP` seul provoque la fermeture du handler.

## 26. `EventLoop` / `ServerHandler` : fds fuités sur chemins d'erreur
**Fichiers :** `srcs/core/EventLoop.cpp`, `srcs/handlers/ServerHandler.cpp`
**Corrections :**
- `~EventLoop` ferme `_epfd` (fuité auparavant).
- `ServerHandler::createSocket` ferme le fd avant de throw sur échec `setsockopt`/`fcntl` (le catch du constructeur ne pouvait pas le fermer, `_fd` n'étant pas encore assigné). `_fd` initialisé à `-1`.

## 27. `handle_accept` : un échec d'`accept` détruisait le socket d'écoute
**Fichier :** `srcs/handlers/ServerHandler.cpp` (`handle_accept`)
**Symptôme :** `accept()` échouant (transitoire) renvoyait `-1` → l'EventLoop **supprimait le ServerHandler** → le port ne répondait plus jamais.
**Correction :** on renvoie toujours `0`. Le fd client accepté reçoit `O_NONBLOCK` (non hérité du socket d'écoute sous Linux) et `FD_CLOEXEC` ; la création du `ClientHandler` est protégée par try/catch.

## 28. Reason phrases HTTP non conformes + `cout` de debug
**Fichiers :** `srcs/utils/utils.cpp`, `srcs/http/ARequestHandler.cpp`, `srcs/utils/utils.cpp` (`decodeHexa`)
**Corrections :**
- Reason phrases alignées sur la RFC : `405 Method Not Allowed`, `413 Payload Too Large`, `501 Not Implemented` ; ajout de `431` et `504`.
- Suppression des `std::cout << "NCODE"...` et `std::cout << "newu"...` qui polluaient la sortie et corrompaient les réponses en cas de redirection de flux.

## 29. Garde-fou mémoire sur les headers
**Fichiers :** `includes/http/HttpParser.hpp`, `srcs/http/HttpParser.cpp`
**Ajout :** si la section headers dépasse `MAX_HEADER_SIZE` (32 Ko) sans terminateur, on renvoie `431` au lieu de laisser `_buffer` grossir sans limite (résilience exigée par le sujet).

---

## Récapitulatif des tests d'exécution (tous verts)

| Test | Résultat |
|------|----------|
| `GET /` sert l'index | `200 text/html` |
| Autoindex `/files/` | listing HTML complet |
| `GET` fichier statique | contenu correct |
| `404` / `403` / `405` / `501` | codes exacts |
| Path traversal `/../../etc/passwd` | `403` (plus de fuite) |
| Redirection `/old-page` | `301` + `Location` |
| CGI GET + query string | sortie du script, en-têtes propres |
| CGI POST body → stdin | body reçu (12 octets) |
| CGI chunked un-chunké | body reçu (11 octets) |
| CGI boucle infinie | serveur reste réactif (`200`), tué au timeout |
| Upload multipart | `201`, fichier écrit |
| Upload `filename=../../evil.txt` | reste dans `www/upload/` |
| DELETE | `204` puis `404` |
| Keep-alive (2 requêtes) | 2× `200` |
| **Pipelining** (2 requêtes, 1 socket) | **2× `200`** |
| Requête malformée | `400`, serveur survit |
| Requête partielle puis close | serveur survit |
| Body > `client_max_body_size` | `413` |
| Multi-serveur / multi-port | 2 ports bindés |
| Erreurs de config (args, fichier, extension) | messages clairs, pas de crash |

Compilation : `-Wall -Wextra -Werror -std=c++98`, **0 warning**.
