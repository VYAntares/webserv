# Rapport d'analyse — webserv

**Projet :** webserv (42) · **Standard :** C++98 · **Date d'analyse :** 2026-07-02
**Méthode :** lecture intégrale du code + compilation + tests d'exécution réels du binaire (curl / netcat / requêtes brutes).

> Ce rapport ne modifie **aucun** code. Il documente l'état réel, vérifié.
> Fichiers associés : [`BUG.md`](BUG.md) (liste des bugs), [`TODO.md`](TODO.md) (plan d'action ordonné), [`BONUS.md`](BONUS.md) (bonus + scripts de test Python).

---

## 1. Résumé exécutif (verdict)

Le projet est **bien plus avancé** que ne le laisse croire l'ancien `srcs/http/AUDIT.md` : POST/upload, DELETE, autoindex, redirections `return`, `error_page`, `client_max_body_size` (413) **fonctionnent** en test réel. L'architecture (pattern Reactor, un seul `epoll`, handlers polymorphes) est propre et conforme à l'esprit du sujet.

**MAIS le projet n'est pas soutenable en l'état.** Trois défauts sont bloquants pour une évaluation 42 :

| # | Gravité | Défaut | Statut |
|---|---------|--------|--------|
| B1 | 🔴 **Fatal éval** | **Path traversal** : `GET /../../../etc/passwd` renvoie le contenu de `/etc/passwd` (vérifié, code 200) | **Confirmé en exécution** |
| B2 | 🔴 **Fatal éval** | **CGI non fonctionnel** : la requête CGI **hang** (aucune réponse, HTTP 000). Le stub ne lit/écrit jamais les pipes, ne `waitpid` pas, ne construit pas de réponse | **Confirmé en exécution** |
| B3 | 🔴 **Important** | **Keep-alive cassé** : `HttpParser::reset()` ne vide ni `_buffer`, ni `_body`, ni `_header`, ni `_req` ; les requêtes pipelinées ne sont jamais re-traitées (deadlock jusqu'au timeout). De plus la réponse code toujours `Connection: close` en dur | **Confirmé en exécution** |

Tant que **B1** (sécurité) et **B2** (le CGI est explicitement demandé au mandatory) ne sont pas corrigés, **la note plafonne** et les bonus ne seront pas évalués.

---

## 2. Ce que j'ai vérifié en exécution (preuves)

Compilation : `make` passe **sans warning** avec `-Wall -Wextra -Werror -std=c++98`. Le binaire tourne, ne crashe pas, survit aux requêtes malformées et à un CGI qui échoue.

| Test | Requête | Résultat observé | Verdict |
|------|---------|------------------|---------|
| GET simple | `GET / HTTP/1.1` | `200 OK` | ✅ OK (contredit l'ancien AUDIT P1 : **pas** de bug `\r`) |
| **Path traversal**  | `GET /../../../../../../../../etc/passwd` | `200 OK` + **contenu de /etc/passwd** | 🔴 **FAILLE** |
| Upload multipart | `POST /up -F file=@f.txt` | `201`, fichier écrit sur disque | ✅ OK |
| DELETE existant | `DELETE /up/f.txt` | `204` | ✅ OK |
| DELETE manquant | `DELETE /up/nope.txt` | `404` | ✅ OK |
| Body > limite | `POST` de 2 Mo (limite 1m) | `413` | ✅ OK |
| Chunked simple | `Transfer-Encoding: chunked` `5\r\nhello\r\n0` | `200` | ✅ cas simple OK |
| **CGI** | `GET /cgi/hello.py` | **hang → timeout, HTTP 000** | 🔴 **CASSÉ** |
| Keep-alive pipeliné | 2 requêtes sur 1 connexion | **1 seule réponse**, la 2ᵉ jamais traitée | 🔴 **CASSÉ** |
| Robustesse | GET après un CGI raté | serveur **toujours vivant**, `200` | ✅ OK (ne crashe pas) |

---

## 3. Architecture (état des lieux)

```
main → ConfigLoader (Lexer → Parser → Validator)
     → ServerHandler (1 par couple IP:port, s'enregistre dans EventLoop)
     → EventLoop::handle_events()   ← boucle epoll unique
            ├─ ServerHandler.handle_accept()  → new ClientHandler
            └─ ClientHandler.handle_input/output()
                   HttpParser → HttpRequest → Router → ARequestHandler
                                                        ├─ StaticHandler (GET/POST/DELETE, autoindex)
                                                        ├─ MultipartHandler (upload)
                                                        ├─ ErrorHandler (error_page, return)
                                                        └─ CGIHandler → CGIProcess  ← STUB non fonctionnel
```

**Points forts :**
- Un **seul** `epoll` pour tous les I/O sockets (conforme au sujet).
- Pattern Reactor propre : `IEventHandler` polymorphe, `EventLoop` ne connaît qu'une interface.
- Pas de check d'`errno` après read/write (conforme à la règle 42).
- Sockets non-bloquants, `SO_REUSEADDR`, gestion des envois partiels (`_sent`), timeout client (30 s), gestion `SIGINT/SIGTERM/SIGQUIT`.
- Parsing de config sérieux (lexer/parser/validator séparés, héritage server→location, checks de doublons `listen`).

**Points faibles structurels :**
- Le **module CGI est un squelette** : `CGIHandler::buildResponse` est commenté, `CGIReadHandler`/`CGIWriteHandler` ont un corps vide, `CGIProcess::~CGIProcess()` ne ferme rien et ne `waitpid` pas. Les fichiers `srcs/handlers/CGIReadHandler.cpp` et `CGIWriteHandler.cpp` sont **vides (0 ligne)**.
- **Sécurité des chemins absente** : aucune normalisation de l'URI avant `root + uri`.
- **Buffer client partagé mal réinitialisé** entre requêtes (keep-alive / pipelining).
- Le CGI, s'il était branché, **fuiterait des fd et laisserait des zombies** (les handlers de pipe ne sont jamais retirés d'epoll quand le client meurt ; le fork n'a pas `FD_CLOEXEC`).

---

## 4. Conformité au sujet (checklist mandatory)

| Exigence du sujet | État |
|-------------------|------|
| Ne crashe jamais | ✅ vérifié (survit CGI raté, requêtes malformées) — **à re-stress-tester** |
| Makefile `NAME/all/clean/fclean/re`, pas de relink inutile | ✅ (⚠️ pas de dépendances headers → un `.hpp` modifié ne recompile pas) |
| `-Wall -Wextra -Werror -std=c++98` | ✅ compile propre |
| C++98 strict | ⚠️ `strtoull` (C99) utilisé — passe ici, **risque portabilité/macOS** (voir BUG B12) |
| 1 seul poll/epoll, lecture ET écriture | ✅ |
| Jamais read/write hors epoll | ⚠️ **sauf CGI** (pipes non pilotés correctement) |
| Pas de check errno après read/write | ✅ |
| Requête ne hang jamais | 🔴 **CGI hang** + **keep-alive deadlock** |
| Compatible navigateur | ⚠️ keep-alive à corriger (nombre de connexions) |
| Codes HTTP corrects | ⚠️ globalement oui ; reason phrases non-RFC, header `Allow:` manquant sur 405 |
| Pages d'erreur par défaut | ✅ |
| Sert un site statique complet | ✅ |
| Upload de fichiers | ✅ (multipart) |
| GET / POST / DELETE | ✅ |
| Stress test / toujours dispo | ⚠️ à faire (keep-alive + copie de `Server` par client coûteuse) |
| Écoute multi-ports | ✅ |
| Config : listen, error_page, body size, méthodes, redirect, root, autoindex, index, upload, CGI | ✅ parsing ; ⚠️ **CGI d'exécution KO** |
| CGI (au moins 1) | 🔴 **non fonctionnel** |
| README conforme (§V) | 🔴 **quasi vide** (3 lignes ; manque Description/Instructions/Resources + mention IA) |

---

## 5. Où regarder en priorité

1. **`srcs/http/Router.cpp:56` `resolvePath()`** → normaliser l'URI, refuser `..` (corrige B1).
2. **Toute la chaîne CGI** (`CGIHandler.cpp`, `CGIProcess.cpp`, `CGIReadHandler`, `CGIWriteHandler`) → à finir (corrige B2).
3. **`srcs/http/HttpParser.cpp:177` `reset()`** + **`ARequestHandler::buildResponse` `Connection:`** → keep-alive (corrige B3).
4. **`srcs/http/HttpParser.cpp:139` `(*end)++`** → UB chunked (corrige B4).
5. Nettoyage debug (`std::cout "NCODE"`, `"newu"`) sur le chemin chaud.
6. **README.md** à réécrire selon le chapitre V.

Détail complet, ligne par ligne, dans [`BUG.md`](BUG.md). Ordre de correction conseillé dans [`TODO.md`](TODO.md).
