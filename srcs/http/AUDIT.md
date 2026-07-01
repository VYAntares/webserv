# Audit complet — Module HTTP + ClientHandler

**Projet :** webserv (42) · **Langage :** C++98 · **Date :** 2026-06-03

Audit en profondeur fichier par fichier du module HTTP et de `ClientHandler`.
Le **CGI n'est pas encore implémenté** et n'est donc pas audité (juste mentionné comme reste à faire).

**Légende de gravité :**
- 🔴 **Critique** — casse le fonctionnement, crash, UB, ou non-conformité C++98 (échec d'éval)
- 🟠 **Important** — bug fonctionnel, faille, ou non-conformité RFC notable
- 🧹 **Nettoyage** — style, perf mineure, code mort, lisibilité

---

## Table des matières

1. [HttpParser](#1-httpparser)
2. [Router](#2-router)
3. [StaticHandler](#3-statichandler)
4. [ErrorHandler](#4-errorhandler)
5. [ClientHandler](#5-clienthandler)
6. [Synthèse globale par priorité](#6-synthèse-globale-par-priorité)
7. [Ce qu'il reste à faire (fonctionnalités manquantes)](#7-ce-quil-reste-à-faire)

---

## 1. HttpParser

**Fichiers :** `srcs/http/HttpParser.cpp`, `includes/http/HttpParser.hpp`, `includes/http/HttpRequest.hpp`

### ✅ Ce qui est fait et fonctionne
- Machine à états (`R_HEADERS → R_BODY/R_CHUNKED → COMPLETE/ERROR`).
- Accumulation du buffer entre `recv` (parsing incrémental).
- Séparation header/body sur `\r\n\r\n`.
- Validation basique de la request-line et lecture du `Content-Length`.
- Squelette du décodage chunked.

### 🔴 Bugs critiques

| # | Ligne | Problème |
|---|-------|----------|
| P1 | `runParsing` 70-72 + `checkFirstLine` 62 | Le `\r` final de la request-line n'est jamais retiré avant `checkFirstLine`. `operator>>` colle `\r` à la version → `_req.version == "HTTP/1.1\r"` → **400 sur toute requête HTTP/1.1 bien formée**. À tester en priorité. |
| P2 | `reset` 155-160 | `reset()` ne vide PAS `_buffer`, `_body`, `_header`, ni `_req`. En keep-alive, la 2ᵉ requête hérite du buffer/body/headers de la 1ʳᵉ → corruption garantie. **Keep-alive fondamentalement cassé.** |
| P3 | `readChunked` 119-120 | `while(*end == ' ') (*end)++;` — `end` pointe dans le buffer de `c_str()` (const). `(*end)++` **modifie le caractère** au lieu d'avancer le pointeur (devait être `end++`). → **UB + boucle infinie** sur toute chunk-extension ou espace. |
| P4 | `readChunked` 116-123 + `setError` 51 | `setError()` en mode `R_CHUNKED` ne change pas l'état (cf. `if (_state != R_CHUNKED)`), et il n'y a pas de `return` après. → l'erreur 400/413 est **inopérante**, le parsing continue sur des données invalides. Une requête chunked en erreur reste active indéfiniment → **DoS** (connexion bloquée). |
| P5 | `headerParser` 96-97 | `if (it->second == "chunked")` teste la **valeur de n'importe quel header** au lieu de la clé `Transfer-Encoding`. Un header arbitraire valant `chunked` bascule en mode chunked ; `Transfer-Encoding: gzip, chunked` n'est jamais détecté. |
| P6 | `headerParser` 90, 96 | Comparaison des **noms de headers sensible à la casse** (`"Content-Length"`). RFC 7230 §3.2 : insensible à la casse. Un client envoyant `content-length:` casse la lecture du body. |
| P7 | `runParsing` 29-34 | En `R_BODY`, tout le `_buffer` est versé dans `_body` (`_body += _buffer; _buffer.clear();`). Une 2ᵉ requête pipelinée gonfle le body → `> _bodyExcepted` → **400 à tort**. Pipelining cassé pour toute requête avec body. |
| P8 | `headerParser` 94 | `strtoull` est **C99/C++11, pas C++98**. En `-std=c++98 -pedantic` = warning/erreur. Échec d'éval 42. À remplacer par `strtoul` ou parsing manuel. |
| P9 | `readChunked` 125-138 | Double comptage de `_bodyReceived` : `size` est ajouté avant réception complète ; si la chunk arrive en plusieurs `recv`, elle est réadditionnée à chaque tour (la chunk n'est pas `erase` tant qu'incomplète). |

### 🟠 Importants
- **1.3 (chunked)** — coexistence `Content-Length` + `Transfer-Encoding` non gérée (request smuggling, RFC 7230 §3.3.3 → devrait rejeter 400).
- **2.3 (chunked)** — `strtol` (signé) pour une chunk-size ; débordement et `-` acceptés ; pas de vérif `errno == ERANGE`.
- **2.4 (chunked)** — le CRLF terminant chaque chunk-data n'est jamais validé.
- **3.3** — `_maxBodySize` jamais comparé à `_bodyReceived` pendant l'accumulation en `R_BODY` (seul `_bodyExcepted` est borné à la lecture des headers).
- **3.5** — `Content-Length` vide (`Content-Length:`) traité comme 0 au lieu de 400 ; overflow `strtoull` non détecté.
- **4.2** — séparateur header cherché en `": "` (espace obligatoire) ; `Host:example.com` (sans espace) rejeté à tort. RFC : OWS optionnel.
- **4.3** — pas de trim de l'OWS de fin de valeur ; `Content-Length: 42 ` → `"42 "` → 400 à tort.
- **4.7** — request-line : `operator>>` accepte espaces multiples et ignore les tokens en trop (`GET / HTTP/1.1 extra`).
- **5.2** — `_req.error = 200` réinitialisé à chaque `runParsing`, peut écraser une erreur posée au recv précédent.
- **5.3** — pas de garde sur `_state == COMPLETE/ERROR` en entrée de `runParsing`.
- **9.3** — pas de validation du header `Host` (obligatoire en HTTP/1.1 → 400 attendu).
- **7.3** — `getReq()` retourne `HttpRequest` **par valeur** → copie de la map de headers + body à chaque appel.

### 🧹 Nettoyage
- `setError` 48 : `std::cout << "setting error"` — log de debug à retirer.
- `_bodyExcepted` → typo pour `_bodyExpected`.
- `runParsing` 11 : paramètre `n` inutilisé (`(void)n`).
- Trailers chunked ignorés après le chunk 0 (résidu dans `_buffer`).
- Headers dupliqués écrasés silencieusement (deux `Content-Length` → devrait être 400).
- Pas de validation des caractères du field-name (doit être un token).
- `getState()` / `getReq()` devraient être `const`.
- Méthodes internes (`setError`, `headerParser`, `readChunked`…) exposées en `public`.
- `HttpRequest::error` non initialisé par défaut (POD struct).
- HTTP/1.0 accepté sans gérer ses différences.

---

## 2. Router

**Fichiers :** `srcs/http/Router.cpp`, `includes/http/Router.hpp`, `includes/config/ConfigStruct.hpp`

### ✅ Ce qui est fait
- Prefix matching (`bestRouteFound`), checks d'existence/permissions, méthodes implémentées/autorisées.
- Ordre globalement correct sur le « cas heureux » (GET d'un fichier existant).

### 🔴 Bugs critiques

| # | Ligne | Problème |
|---|-------|----------|
| R1 | `bestRouteFound` 86-88 | Matching cassé : `path.find(shorturi)==0 && path==shorturi` est redondant et revient à une **égalité stricte**, pas un prefix matching. La vraie logique serait `shorturi.find(path)==0`. Ça « marche » seulement grâce à la troncature segment par segment de l'URI. |
| R2 | `resolvePath` 40 | **Path traversal** : `loc->root + uri` concaténé brut, sans normalisation. `GET /../../etc/passwd` sort de `root`. Faille de sécurité majeure. |
| R3 | `resolvePath` 46-48 | Branche `autoindex == 1` non implémentée : juste un `std::cout`, puis retourne le chemin du **répertoire** comme si c'était un fichier → `StaticHandler` tente de lire un dossier → body vide en 200. |
| R4 | `route` 16 | `new ErrorHandler(loc, 404)` avec `loc == NULL` garanti (on est dans le `if (!loc)`). Inoffensif tant que `ErrorHandler` ignore `loc`, mais **crash dès que les error_page custom seront implémentées**. |
| R5 | `route` (absent) | **`return_path` / redirections jamais consultées.** Une location configurée en redirect 301/302 sert un fichier statique ou 404. Fonctionnalité manquante. |
| R6 | `route` (absent) | **`max_body_client` jamais vérifié.** Aucun 413 généré pour un POST/upload trop gros. |

### 🟠 Importants
- **1.2** — `shorturi.erase(i)` avec `i == npos` non protégé (boucle infinie si la logique évolue). Aujourd'hui sauvé par chance car l'URI commence par `/`.
- **1.3** — `uri[0]` sur URI vide = UB en C++98 (`operator[](size())` non-const). Manque `uri.empty() ||`.
- **2.2** — le prefix de la location n'est jamais retiré de l'URI (sémantique `root` vs `alias` nginx à clarifier).
- **2.3bis** — `autoindex == -1` (non défini) traité comme `off` au lieu d'hériter de la valeur du Server.
- **3.1 / 4.1** — incohérence POST : `fileFound` court-circuite (return 1) mais `forbiddenAccess(W_OK)` échoue sur fichier inexistant → **403 au lieu d'upload**.
- **4.1** — `forbiddenAccess` : `mode = 0` (F_OK) par défaut si méthode inconnue → teste l'existence au lieu de refuser. Non atteignable aujourd'hui (filtré par `methodImplemented`) mais piège si l'ordre change.
- **5.2** — `methodAllowed` : `allowed_methods` vide = **tout autorisé** (DELETE inclus). À valider contre le sujet.
- **6.2** — ErrorHandler 403 (ligne 29) et 405 (ligne 32) appelés **sans `loc`** → pas d'error_page perso. Incohérent avec lignes 20/26.
- **6.3** — **ordre des checks** : `methodAllowed` (405) est testé après `fileFound` (404) → un DELETE non autorisé sur fichier inexistant renvoie 404 au lieu de 405. La RFC veut 405 avant 404. `methodImplemented` (501) devrait aussi venir plus tôt.
- **6.6** — CGI non routé (`//if (isCgi())`) — attendu, à tracer.
- **7.1** — ownership des handlers `new` reposant entièrement sur l'appelant (risque de fuite).

### 🧹 Nettoyage
- `resolvePath` 42 : `path[path.length()-1]` sans garde sur chaîne vide.
- Prédicats renvoyant `int` au lieu de `bool` (`fileFound`, `forbiddenAccess`, `methodAllowed`).
- `req.error != 200` : `200` codé en dur.
- Includes morts : `<cstring>`, `<cstdio>`.
- `std::cout << "throw list"` (ligne 48) à retirer.
- Casts C-style `(int)` (lignes 88, 90) ; `-Wsign-compare`.
- `resolvePath` retourne `const std::string` par valeur (inutile).

---

## 3. StaticHandler

**Fichiers :** `srcs/http/StaticHandler.cpp`, `includes/http/StaticHandler.hpp`

### ✅ Ce qui est fait
- `handleGet` : ouverture + lecture via `rdbuf`, détection MIME par extension.
- `buildResponse` (status line, Content-Type, Content-Length).
- `getReason` couvre les codes courants.

### 🔴 Bugs critiques

| # | Ligne | Problème |
|---|-------|----------|
| S1 | 10 | Condition de redirect `if (!loc.return_path.first \|\| !loc.return_path.second.empty())`. `first` est un **code** (-1 par défaut). `!first` n'est vrai que si `first==0`. La condition « marche par chance » via `!second.empty()` mais est sémantiquement fausse. Devrait être `if (loc.return_path.first != -1)`. |
| S2 | 92-96 + 43-52 | `handleReturn` n'émet **aucun header `Location:`** → la redirection HTTP est **non fonctionnelle** (le navigateur affiche le body au lieu de rediriger). `buildResponse` ne sait pas émettre `Location`. De plus `_ncode = first` non validé. |
| S3 | 73-75 | `handleGet` : si `!file.is_open()`, `return` sans toucher `_ncode` (reste 200) → **200 OK avec body vide** au lieu de 404/403. Aussi : TOCTOU et répertoire lu comme fichier. |
| S4 | 83-86 | `handlePost` ne fait **aucun upload** : `upload_store` jamais lu, aucun fichier écrit, jamais de 201. Juste des `std::cout`. |
| S5 | 88-90 | `handleDelete` ne supprime **rien** (`unlink` absent), jamais de 204, **faux succès silencieux**. |
| S6 | 7 (ctor) | `_req = &req` pointe vers une `HttpRequest` **locale** de `ClientHandler::_handleComplete`. Dangling dès le retour de cette fonction → **use-after-free latent** (inoffensif aujourd'hui car `buildResponse` est appelé immédiatement, mais piège dangereux). |

### 🟠 Importants
- **6** — headers manquants : `Location` (3xx), `Date`, `Server`, et surtout `Allow:` obligatoire sur un 405 (RFC 7231 §6.5.5).
- **6** — `Connection: close` codé en dur (ligne 48) → **contredit la logique keep-alive** du ClientHandler.
- **6** — `Content-Type:` émis même vide → header invalide possible.
- **7** — `mime_types` globale mutable au scope fichier (devrait être `const` / encapsulée).
- **8** — reason phrases non conformes RFC (`Method not allowed` → `Method Not Allowed`, `Body size too large` → `Payload Too Large`…) ; 303/307/308 absents.
- **10** — init statique non-triviale au scope fichier (risque d'ordre d'init entre TU).
- **11** — fichier lu intégralement en mémoire (pas de streaming) + pas de `try/catch` autour de `bad_alloc`.

### 🧹 Nettoyage
- HTML mal formé dans `handleReturn` (`</html></body></h1>`).
- `getType` : double lookup `count` + `operator[]` ; recherche d'extension case-sensitive et naïve.
- `std::cout` de debug dans `handlePost`/`handleDelete`.
- `_type("")` redondant dans la member-init list.
- Indentation mixte tabs/espaces dans `getReason`.

---

## 4. ErrorHandler

**Fichiers :** `srcs/http/ErrorHandler.cpp`, `includes/http/ErrorHandler.hpp`

### ✅ Ce qui est fait
- Deux constructeurs, génération d'une page HTML d'erreur, `buildResponse` correct sur le fond.

### 🔴 Bugs critiques

| # | Ligne | Problème |
|---|-------|----------|
| E1 | 4-12 | Le constructeur `ErrorHandler(const Location*, int)` fait `(void)loc;` : **error_page custom non implémentées**. Les deux constructeurs sont fonctionnellement identiques. Fonctionnalité phare du sujet absente. |
| E2 | 4 (+ Router 16) | Aucune protection `loc == NULL` alors que le Router passe explicitement `NULL`. **Segfault garanti dès que la lecture de `loc->error_page` sera ajoutée.** À traiter avec E1. |

### 🟠 Importants
- **3** — `getReason` : `200` absent, codes 3xx/502/503/504 manquants, reason phrases non RFC, fallback `"Unknown"` (un 500 serait plus sûr).
- **4** — `buildResponse` : headers `Date` et `Server` manquants ; `getReason()` appelé 3× au lieu d'1.

### 🧹 Nettoyage
- `itos` (ligne 32) dupliquée : `toString` existe déjà dans `ConfigStruct.hpp` **et** dans `Lexer.cpp` (3 implémentations). De plus `itos` est un symbole global non-`static` → risque ODR.
- `<iostream>` inclus inutilement dans le header.

---

## 5. ClientHandler

**Fichiers :** `srcs/handlers/ClientHandler.cpp`, `includes/handlers/ClientHandler.hpp`

### ✅ Ce qui est fait — globalement solide
- Cycle de vie clair, keep-alive vs close, registration EventLoop READ/WRITE.
- `handle_output` gère correctement les **envois partiels** (`_sent`).
- `n <= 0 → return -1` conforme à la règle 42 (pas de check `errno`).
- `_reset` : `delete _rh; _rh = NULL` (pas de double-free), destructeur cohérent avec `EventLoop`.
- `_lastActivity` correctement utilisé par `EventLoop::checkTimeOut` (timeout 30s fonctionnel).

### 🔴 Bugs critiques

| # | Ligne | Problème |
|---|-------|----------|
| C1 | `_reset` 25 ↔ `HttpParser::reset` | Racine commune avec P2 : `reset()` partiel → en keep-alive, la 2ᵉ requête est corrompue (body concaténé, headers fusionnés) ou perdue (données pipelinées dans `_buffer` jamais re-traitées → deadlock jusqu'au timeout). **Keep-alive cassé.** |
| C2 | ctor 39 | `register_handler` peut **throw** (`epoll_ctl` échoue). Si throw dans le constructeur → destructeur non appelé → `close(_fd)` jamais fait → **fuite de fd** + exception probablement non gérée dans la boucle accept. |

### 🟠 Importants
- **1.2** — `std::string data(buf, n)` puis `_parser.runParsing(data, n)` : copie inutile (le param `n` est ignoré dans le parser).
- **1.3/1.4** — une seule requête traitée par appel ; les données déjà dans `_buffer` (pipelining) ne sont jamais re-déclenchées (pas dans la socket).
- **2.1** — si `_response` est vide, `send(...,0,...)` retourne 0 → `return -1` → fermeture intempestive.
- **4.1** — détection `Connection`/`keep-alive` sensible à la casse (RFC = insensible) ; `Connection: Keep-Alive` en HTTP/1.0 mal géré.
- **4.2** — `getReq()` par valeur → copie du body à chaque requête.
- **5.2** — pas de garde `if (_rh)` avant `_rh->buildResponse()` (crash si `route` renvoie NULL).
- **6.2** — couplage implicite : `delete` d'un ClientHandler sans `remove_handler` préalable corromprait epoll. Contrat respecté par EventLoop mais non garanti par la classe.
- **7.1** — `Server _server` **copié par valeur** : toute la config (vector de Locations avec maps) dupliquée par client. Devrait être `const Server*`. Impacte le stress test.
- **10.1** — fuite conditionnelle de `_rh` si `COMPLETE` survient deux fois sans `_reset` intermédiaire.

### 🧹 Nettoyage
- Logs `std::cout` en dur (lignes 40, 48, 67) sur stdout → ralentissent le stress test.
- `_handleError` quasi identique à `_handleComplete` (duplication).
- Buffer `char buf[4096]` (mineur).
- `#pragma once` (non-standard, toléré par 42).

---

## 6. Synthèse globale par priorité

### 🔴 Critiques — à corriger absolument

| Réf | Fichier | Problème |
|-----|---------|----------|
| P1 | HttpParser:70-72 | `\r` collé à la version → 400 sur toute requête HTTP/1.1 |
| **P2 / C1** | HttpParser:155 / ClientHandler:25 | `reset()` ne vide pas `_buffer`/`_body`/`_header`/`_req` → **keep-alive cassé** |
| P3 | HttpParser:119-120 | `(*end)++` écrit dans `c_str()` (UB) + boucle infinie chunked |
| P4 | HttpParser:116-123 | `setError` en chunked sans effet ni `return` → DoS |
| P5 | HttpParser:96 | détection chunked sur la mauvaise donnée |
| P6 | HttpParser:90,96 | noms de headers case-sensitive |
| P7 | HttpParser:29-34 | pipelining body → 400 à tort |
| P8 | HttpParser:94 | `strtoull` non C++98 → **échec d'éval** |
| R1 | Router:86 | prefix matching incorrect (égalité stricte) |
| R2 | Router:40 | **path traversal** `root + uri` non normalisé |
| R3 | Router:46 | autoindex non implémenté, dossier servi comme fichier |
| R4 | Router:16 | `ErrorHandler(NULL, …)` (crash futur) |
| S1 | StaticHandler:10 | condition de redirect fausse |
| S2 | StaticHandler:92 | redirect sans header `Location:` |
| S3 | StaticHandler:73 | échec d'ouverture → 200 body vide |
| S4/S5 | StaticHandler:83/88 | POST/DELETE ne font rien |
| S6 | StaticHandler:7 | `_req` dangling (use-after-free latent) |
| E1/E2 | ErrorHandler:4 | error_page non implémentées + `loc==NULL` non protégé |
| C2 | ClientHandler:39 | exception en ctor → fuite de fd |

### 🟠 Importants
Voir le détail par fichier. Points transverses notables :
- **Headers case-insensitive** à normaliser partout (parser + keep-alive).
- **Headers de réponse** incomplets (`Date`, `Server`, `Location`, `Allow` sur 405).
- **Ordre des checks du Router** (405 doit primer sur 404 ; 501 plus tôt).
- **Copies coûteuses** : `getReq()` par valeur, `Server` copié par client.
- **`max_body_client`** non vérifié (Router) / non borné pendant l'accumulation (parser).

### 🧹 Nettoyage
- Retirer tous les `std::cout` de debug (HttpParser:48, Router:48, StaticHandler:84/89, ClientHandler:40/48/67).
- Factoriser `itos`/`toString`/`getReason` (3 implémentations dispersées).
- `mime_types` global → `static` local const.
- Includes morts, casts C-style, prédicats `int`→`bool`.

---

## 7. Ce qu'il reste à faire

Fonctionnalités non implémentées ou à compléter (hors corrections de bugs) :

- **POST / upload** — écriture des fichiers via `upload_store`, parsing `multipart/form-data`, réponse 201.
- **DELETE** — suppression réelle (`unlink`), réponse 204.
- **Autoindex** — génération de la page HTML de listing de répertoire (handler dédié ou logique dans StaticHandler).
- **error_page custom** — lecture de `loc->error_page` dans ErrorHandler (avec garde `loc == NULL`).
- **Redirections complètes** — `return_path` consulté par le Router + header `Location:` émis avec le bon code.
- **`max_body_client`** — vérification effective → 413.
- **Host obligatoire** (HTTP/1.1) — 400 si absent.
- **CGI** — entièrement à faire (stub `CGIHandler`) ; `cgi_pass`, fork/exec, variables d'environnement, pipes. *(Exclu du présent audit.)*
- **Sécurité** — normalisation/refus des `..` (path traversal), validation des tokens.
- **Pipelining / keep-alive robuste** — séparer « octets de la requête courante » de « octets de la suivante » dans le buffer.

---

*Audit généré par analyse multi-agents (un agent par fichier), compilé et mis au propre.*
