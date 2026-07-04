# BUG.md — Liste des bugs (vérifiés & suspectés)

Légende gravité :
- 🔴 **Critique** : crash, faille, non-conformité éval, ou fonctionnalité mandatory KO.
- 🟠 **Important** : bug fonctionnel ou écart RFC visible.
- 🧹 **Nettoyage** : style, perf, code mort, lisibilité.

Statut : **[VÉRIFIÉ]** = reproduit en exécution · **[CODE]** = certain par lecture du code · **[SUSPECT]** = plausible, à confirmer.

---

## 🔴 CRITIQUES

### B1 — Path traversal (fuite de fichiers hors `root`) · [VÉRIFIÉ]
**Fichier :** `srcs/http/Router.cpp:56-66` (`resolvePath`) — concaténation brute `loc->root + uri`.
**Preuve :** `GET /../../../../../../../../etc/passwd HTTP/1.1` → **`200 OK` + contenu de `/etc/passwd`**.
**Cause :** aucune normalisation ni rejet des segments `..`. `root + "/../../etc/passwd"` est résolu par le noyau vers un chemin hors racine.
**Impact :** faille de sécurité majeure → **note 0 possible** en éval sécurité.
**Correctif :** normaliser l'URI (résoudre `.`/`..` en interne) **avant** concaténation, et refuser toute requête qui, après normalisation, sort de `root` (répondre `403`/`404`). Décoder le `%xx` d'abord (sinon `%2e%2e` contourne).

### B2 — CGI non fonctionnel (la requête hang) · [VÉRIFIÉ]
**Fichiers :** `srcs/http/CGIHandler.cpp`, `srcs/cgi/CGIProcess.cpp`, `srcs/handlers/CGIReadHandler.cpp` (**vide**), `srcs/handlers/CGIWriteHandler.cpp` (**vide**).
**Preuve :** `GET /cgi/hello.py` → **timeout, HTTP 000**, aucune réponse.
**Causes cumulées :**
1. `CGIHandler::buildResponse()` est **commenté** (`CGIHandler.cpp:28`) → la classe hérite du `buildResponse` de base avec `_ncode` **non initialisé** (valeur indéterminée dans la réponse).
2. `CGIWriteHandler::handle_output()` et `CGIReadHandler::handle_input()` **retournent 0 sans rien faire** : le body n'est **jamais** écrit dans le pipe stdin du CGI, la sortie du CGI n'est **jamais** lue.
3. Aucun `waitpid()` → **zombie** ; `CGIProcess::~CGIProcess()` (ligne 201) est **vide** → `_write_fd`/`_read_fd` **jamais fermés** (fuite de fd).
4. Le client n'est jamais rebasculé en `WRITE_EVENT` avec la sortie CGI → il attend indéfiniment.
5. `fork()` sans `FD_CLOEXEC` sur le fd client/listen → l'enfant CGI **hérite** de ces sockets.
**Impact :** fonctionnalité **mandatory** absente → plafonne la note, bonus non évalués.
**Correctif :** implémenter réellement la boucle CGI pilotée par epoll (écrire le body via `CGIWriteHandler`, lire la sortie via `CGIReadHandler`, non-bloquant), parser les headers CGI (`Status:`, `Content-Type`), gérer EOF = fin de sortie, `waitpid` non-bloquant, fermer tous les fd, `FD_CLOEXEC`, timeout CGI, rattacher la réponse au `ClientHandler`.

### B3 — Keep-alive cassé + réponse toujours `Connection: close` · [VÉRIFIÉ]
**Fichiers :** `srcs/http/HttpParser.cpp:177-182` (`reset`), `srcs/http/ARequestHandler.cpp:46`, `srcs/handlers/ClientHandler.cpp:20-26`.
**Preuve :** 2 requêtes pipelinées sur 1 connexion → **1 seule réponse**, la 2ᵉ jamais servie (deadlock jusqu'au timeout 30 s).
**Causes :**
1. `HttpParser::reset()` **ne vide pas** `_buffer`, `_body`, `_header`, `_req`. En keep-alive, la 2ᵉ requête hérite du body/headers de la 1ʳᵉ → corruption ou données pipelinées perdues.
2. `ARequestHandler::buildResponse` écrit **`Connection: close` en dur** → contredit la logique `_keepAlive` de `ClientHandler`. Le client ferme donc toujours.
3. Les octets déjà présents dans `_buffer` (pipelining) ne re-déclenchent pas `runParsing` (rien n'arrive sur la socket) → jamais traités.
**Impact :** « la requête ne doit jamais hang » (sujet) violé ; keep-alive non conforme HTTP/1.1.
**Correctif :** dans `reset()`, remettre `_req = HttpRequest()`, vider `_body`/`_header`, **conserver** `_buffer` pour re-parser le reliquat pipeliné, et relancer `runParsing("")` après reset. Faire émettre par `buildResponse` le bon `Connection:` (keep-alive/close) transmis par le handler.

### B4 — UB + boucle infinie sur chunk-extension · [CODE]
**Fichier :** `srcs/http/HttpParser.cpp:139-140`.
```cpp
while(*end == ' ' || *end == '\t')
    (*end)++;      // ← modifie le CARACTÈRE, pas le pointeur
```
`end` pointe dans le buffer renvoyé par `c_str()` (const) ; `(*end)++` **écrit dedans** (UB) et n'avance jamais → **boucle infinie** dès qu'un espace/tab suit la taille de chunk. Devait être `end++`.
**Impact :** DoS potentiel sur requête chunked avec extension ; UB.
**Correctif :** `end++;` (avancer le pointeur).

### B5 — `setError()` inopérant en mode chunked · [CODE]
**Fichiers :** `srcs/http/HttpParser.cpp:47-52` + `124-167`.
`setError()` ne change pas l'état si `_state == R_CHUNKED` (`if (_state != R_CHUNKED)`), et `readChunked` ne `return` pas après un `setError`. Un chunk invalide (400) ou trop gros (413) **n'interrompt pas** le parsing : la boucle continue sur des données invalides.
**Impact :** requête chunked en erreur peut rester active → connexion bloquée (DoS) jusqu'au timeout.
**Correctif :** poser l'état `ERROR` immédiatement et sortir de la boucle après toute erreur.

### B6 — `resolvePath` : accès `path[len-1]` sans garde chaîne vide · [CODE]
**Fichier :** `srcs/http/Router.cpp:59` (`path[path.length() - 1]`).
Si `root` et `uri` produisent une chaîne vide (config incohérente), `path[-1]` = UB. `bestRouteFound` (`Router.cpp:94` `uri[0]`) a le même risque si `uri` est vide.
**Correctif :** garde `if (!path.empty())` / `if (uri.empty()) return NULL;`.

---

## 🟠 IMPORTANTS

### B7 — En-têtes HTTP sensibles à la casse · [CODE]
**Fichier :** `srcs/http/HttpParser.cpp:97,103,105` + `ClientHandler.cpp:57`.
`"Content-Length"`, `"Connection"`, détection `chunked` comparés en respectant la casse. RFC 7230 §3.2 : noms de headers **insensibles à la casse**. Un client envoyant `content-length:` ou `connection: keep-alive` casse la logique.
**Correctif :** normaliser les noms de header en minuscules à l'insertion, comparer en minuscules.

### B8 — Détection `chunked` sur la mauvaise donnée · [CODE]
**Fichier :** `srcs/http/HttpParser.cpp:103` `if (it->second == "chunked")`.
Teste la **valeur de n'importe quel header** au lieu de la clé `Transfer-Encoding`. Un header quelconque valant `chunked` bascule en mode chunked ; `Transfer-Encoding: gzip, chunked` n'est pas détecté.
**Correctif :** tester `it->first == "transfer-encoding"` (insensible casse) et chercher `chunked` dans la valeur.

### B9 — Ordre des checks Router : 405 après 404 · [CODE]
**Fichier :** `srcs/http/Router.cpp:22-32`.
`fileExist` (404) est testé **avant** `methodAllowed` (405). Un `DELETE` non autorisé sur fichier inexistant renvoie **404 au lieu de 405**. `methodImplemented` (501) devrait aussi passer plus tôt. De plus `methodAllowed` est appelé **deux fois** (lignes 25 et 31 — doublon).
**Correctif :** ordre : 501 (non implémentée) → 405 (non autorisée) → 404 (absente) → 403. Supprimer l'appel dupliqué.

### B10 — En-têtes de réponse manquants · [CODE]
**Fichier :** `srcs/http/ARequestHandler.cpp:36-48`.
Manquent : `Allow:` (obligatoire sur 405, RFC 7231 §6.5.5), `Date:`, `Server:`. `Content-Type:` peut être émis vide.
**Correctif :** ajouter `Allow` sur 405 (liste des méthodes de la location), `Date`, `Server`.

### B11 — `allowed_methods` vide = tout autorisé (DELETE inclus) · [CODE]
**Fichier :** `srcs/http/Router.cpp:123-125`.
Si aucune méthode n'est configurée, `methodAllowed` renvoie 1 pour tout (y compris DELETE). À valider : nginx interdit par défaut hors GET. Peut permettre des DELETE non voulus.
**Correctif :** définir une politique par défaut explicite (ex. GET seul) ou documenter le choix.

### B12 — `strtoull` : C99, pas C++98 strict · [VÉRIFIÉ partiel]
**Fichier :** `srcs/http/HttpParser.cpp:101`.
Passe **sur ce setup** (GCC/glibc, même en `-pedantic-errors`), mais `strtoull` n'existe pas en `<cstdlib>` C++98 standard → **risque sur macOS/clang strict** ou correcteur pointilleux.
**Correctif :** remplacer par `strtoul` + contrôle d'overflow, ou parsing manuel.

### B13 — `Content-Length` vide / overflow non gérés · [CODE]
**Fichier :** `srcs/http/HttpParser.cpp:97-101`.
`Content-Length:` (vide) → `strtoull("")` = 0 (au lieu de 400). Pas de détection d'overflow. Coexistence `Content-Length` + `Transfer-Encoding` non rejetée (request smuggling, RFC 7230 §3.3.3).
**Correctif :** rejeter valeur vide/non numérique en 400, refuser la coexistence des deux headers.

### B14 — Séparateur header `": "` (espace obligatoire) · [CODE]
**Fichier :** `srcs/http/HttpParser.cpp:89` `line.find(": ")`.
`Host:example.com` (sans espace) → `sep == npos` → **400 à tort**. L'OWS est optionnel (RFC). Idem pas de trim de l'OWS en fin de valeur (`Content-Length: 42 ` → `"42 "` → 400).
**Correctif :** couper sur le premier `:`, puis trim des espaces autour de la valeur.

### B15 — Validation du header `Host` absente (HTTP/1.1) · [CODE]
**Fichier :** `srcs/http/HttpParser.cpp` (headerParser).
HTTP/1.1 impose `Host:` → `400` attendu s'il manque. Non vérifié.
**Correctif :** si version 1.1 et pas de `Host` → 400.

### B16 — `Server _server` copié par valeur dans chaque client · [CODE]
**Fichier :** `includes/handlers/ClientHandler.hpp:24` + `ClientHandler.cpp` ctor.
Toute la config (vecteur de `Location` avec maps) est **dupliquée par connexion**. Impact mémoire/perf sous stress test.
**Correctif :** stocker `const Server*` (durée de vie du `ServerHandler` garantie).

### B17 — Fuite de fd si exception dans le ctor de `ClientHandler` · [CODE]
**Fichier :** `srcs/handlers/ClientHandler.cpp:33-41`.
`register_handler` peut throw (`epoll_ctl`). Le destructeur n'étant pas appelé sur objet partiellement construit → `close(_fd)` jamais fait → **fuite de fd**, exception non gérée dans `handle_accept`.
**Correctif :** try/catch dans le ctor (fermer `_fd` avant re-throw) ou RAII sur le fd.

### B18 — Pointeurs `_req` pendants dans les handlers · [CODE/SUSPECT]
**Fichiers :** `StaticHandler` / `MultipartHandler` stockent `const HttpRequest* _req` ; `CGIProcess`/`CGIHandler` gardent des **références** `const HttpRequest&`, `std::string&`.
`ClientHandler::_handleComplete` construit `HttpRequest req` **locale**, la passe à `Router::route`, puis appelle `buildResponse()` immédiatement — OK aujourd'hui. Mais pour le CGI (asynchrone), la requête locale **disparaît** avant que le CGI ait fini → **use-after-free** dès que le CGI sera branché.
**Correctif :** faire porter la durée de vie de `HttpRequest` par `ClientHandler` (membre), pas une locale, tant que des handlers asynchrones la référencent.

### B19 — Handlers CGI jamais retirés d'epoll quand le client meurt · [CODE]
**Fichiers :** `EventLoop.cpp` + `CGIHandler`.
Quand un `ClientHandler` est détruit, ses `CGIReadHandler`/`CGIWriteHandler` (objets séparés sur les fd de pipe) **restent enregistrés** dans epoll et ne sont pas delete → fuite + fd de pipe pendants. `CGIHandler::~CGIHandler` fait `delete _process` mais `~CGIProcess` ne ferme rien.
**Correctif :** à concevoir avec B2 : lier le cycle de vie des handlers CGI à celui du client, tout retirer/fermer proprement.

### B20 — `MultipartHandler` : chemin d'upload non sécurisé · [CODE]
**Fichier :** `srcs/http/MultipartHandler.cpp:8` `_path + "/" + i->second.filename`.
Le `filename` vient du client (`Content-Disposition`) et n'est **pas** assaini → un `filename="../../evil"` écrit hors `upload_store` (path traversal en écriture). Écrit dans `_path` (le path résolu), pas dans `loc.upload_store` — vérifier la cohérence. Pas de check d'échec d'ouverture du `ofstream`.
**Correctif :** ne garder que le basename du filename, écrire dans `upload_store`, vérifier l'ouverture du fichier.

---

## 🧹 NETTOYAGE

- **Debug sur le chemin chaud :** `srcs/http/ARequestHandler.cpp:35` `std::cout << "NCODE"...` (à **chaque** réponse) et `srcs/utils/utils.cpp:124` `std::cout << "newu"...` (à chaque décodage %) → **ralentit le stress test**, pollue stdout. À retirer.
- **`itos` / `toString` en triple** : `utils.cpp`, `ConfigStruct.hpp`, `Lexer.cpp` (symbole global non-`static` → risque ODR). Factoriser.
- **`mime_types`** : global mutable au scope fichier (`utils.cpp:3`) → préférer `static const` local.
- **Casts C-style** (`(int)`, `(char*)`) et prédicats renvoyant `int` au lieu de `bool` (`fileFound`, `forbiddenAccess`, `methodAllowed`).
- **`getState()` / `getReq()`** non `const` ; `getReq()` renvoie par **valeur** (copie body + map à chaque appel).
- **Méthodes internes du parser** (`setError`, `headerParser`, `readChunked`…) exposées en `public`.
- **`_bodyExcepted`** → typo pour `_bodyExpected`.
- **HTML mal formé** dans `ARequestHandler::handleReturn` : `</html></body></h1>` (balises inversées).
- **Reason phrases non-RFC** : `Method not allowed` → `Method Not Allowed`, `Body size too large` → `Payload Too Large` ; 302/303/307/308 partiels.
- **`Makefile`** : pas de dépendances `.hpp` → modifier un header ne recompile pas les `.cpp` (rebuild manquant, pas un relink inutile). Ajouter `-MMD -MP`.
- **`webserv-studies/`** et les nombreux `.md` (`AUDIT.md`, `SUMMARY.md`, `reactorPattern.md`) : à exclure du rendu si non demandés (ne pas polluer le dépôt évalué).
- **`test.html`** à la racine : fichier de test orphelin.
- Fichiers **vides** `srcs/handlers/CGIReadHandler.cpp` / `CGIWriteHandler.cpp` (compilent à vide car tout est inline dans le `.hpp`).

---

## Note sur l'ancien `srcs/http/AUDIT.md`

Cet audit est **daté** et partiellement **obsolète** : plusieurs bugs qu'il liste comme critiques sont **infirmés** ou **déjà corrigés** :
- **P1** (« 400 sur tout HTTP/1.1 » à cause du `\r`) : **FAUX aujourd'hui** — testé, renvoie 200 (`operator>>` traite `\r` comme séparateur).
- **S4/S5** (« POST/DELETE ne font rien ») : **corrigés** — upload 201 et DELETE 204 vérifiés.
- **R3** (« autoindex non implémenté ») : **implémenté** (`throwList`).
- **E1/S2** (return/error_page absents) : **implémentés**.

À ne **pas** utiliser tel quel comme feuille de route. Le présent `BUG.md` reflète l'état réel testé le 2026-07-02.
