# TODO.md — Plan d'action ordonné

Objectif : rendre webserv **soutenable et parfait** pour l'éval 42, puis débloquer les bonus.
Les identifiants `Bxx` renvoient à [`BUG.md`](BUG.md).

> Rappel : les bonus (cookies/sessions, multi-CGI) ne sont évalués **que si** tout le mandatory passe sans faille.

---

## Étape 0 — Sécuriser (bloquant éval sécurité)

- [ ] **B1 — Path traversal.** Dans `Router::resolvePath` : décoder `%xx` en premier, **normaliser** l'URI (résoudre `.` et `..` en interne), puis vérifier que le chemin final reste sous `root`. Sinon `403`/`404`.
  - Test de non-régression : `GET /../../../../etc/passwd` doit renvoyer 403/404, **pas** 200.
- [ ] **B20 — Upload traversal.** Dans `MultipartHandler`, ne garder que le **basename** du `filename` client, écrire dans `upload_store`, vérifier l'ouverture du fichier.

## Étape 1 — Rendre le CGI fonctionnel (mandatory)

- [ ] **B2 — Boucle CGI pilotée par epoll (non-bloquante) :**
  - [ ] `CGIWriteHandler` : écrire le body de la requête dans le pipe stdin du CGI par morceaux, fermer le fd d'écriture une fois tout envoyé (EOF pour le CGI).
  - [ ] `CGIReadHandler` : lire la sortie du CGI, accumuler ; EOF = fin de sortie.
  - [ ] Parser la sortie CGI : headers CGI (`Status:`, `Content-Type:`, etc.) → construire la réponse HTTP ; si pas de `Content-Length`, EOF marque la fin (cf. sujet).
  - [ ] `waitpid` **non-bloquant** pour récupérer l'enfant (pas de zombie).
  - [ ] Rebrancher la réponse CGI sur le `ClientHandler` (repasser en `WRITE_EVENT`).
  - [ ] **Timeout CGI** (le sujet exige qu'aucune requête ne hang) → tuer le process + `504`.
- [ ] **B19 — Cycle de vie** : retirer d'epoll et fermer **tous** les fd/pipe quand le client ou le CGI se termine ; `~CGIProcess` doit fermer `_write_fd`/`_read_fd`.
- [ ] `fork()` : poser `FD_CLOEXEC` sur les sockets pour que l'enfant CGI n'en hérite pas.
- [ ] **B18** — faire porter la durée de vie de `HttpRequest` par `ClientHandler` (membre), pas une locale, tant que le CGI la référence.
- [ ] **B2.1** — implémenter `CGIHandler::buildResponse` (retirer le commentaire) OU intégrer le CGI hors de `ARequestHandler` ; initialiser `_ncode`.
  - Test : `GET /cgi/hello.py` renvoie 200 + sortie du script ; `POST` avec body transmis via stdin ; chunked un-chunké avant passage au CGI.

## Étape 2 — Keep-alive & parsing robustes

- [ ] **B3 — `HttpParser::reset()`** : réinitialiser `_req = HttpRequest()`, vider `_body`/`_header`, **conserver** `_buffer` (reliquat pipeliné) et relancer le parsing dessus après reset.
- [ ] **B3 — `Connection:`** : `buildResponse` doit émettre `keep-alive` ou `close` selon la décision du `ClientHandler` (ne plus coder `close` en dur).
- [ ] **B4 — `(*end)++` → `end++`** dans `readChunked`.
- [ ] **B5 — `setError` en chunked** : passer en `ERROR` immédiatement et sortir de la boucle.
- [ ] **B6** — gardes chaîne vide dans `resolvePath` / `bestRouteFound`.
- [ ] **B13** — `Content-Length` vide/non numérique → 400 ; refuser `Content-Length` + `Transfer-Encoding` simultanés.
- [ ] **B15** — `Host` manquant en HTTP/1.1 → 400.

## Étape 3 — Conformité HTTP / RFC

- [ ] **B7** — headers **insensibles à la casse** (normaliser à l'insertion).
- [ ] **B8** — détecter `chunked` sur la clé `Transfer-Encoding`, pas sur une valeur quelconque.
- [ ] **B9** — ordre des checks Router : 501 → 405 → 404 → 403 ; supprimer l'appel `methodAllowed` dupliqué.
- [ ] **B10** — ajouter `Allow:` sur 405, `Date:`, `Server:` ; ne pas émettre `Content-Type:` vide.
- [ ] **B14** — séparateur header sur premier `:`, trim de l'OWS.
- [ ] Reason phrases conformes RFC (`Method Not Allowed`, `Payload Too Large`, …).
- [ ] **B11** — définir la politique par défaut de `allowed_methods` (documenter).

## Étape 4 — Perf & robustesse (stress test)

- [ ] **B16** — `ClientHandler` : `const Server*` au lieu de copie par valeur.
- [ ] **B17** — try/catch dans le ctor `ClientHandler` (fermer `_fd` avant re-throw).
- [ ] Retirer les `std::cout` de debug (`ARequestHandler.cpp:35` « NCODE », `utils.cpp:124` « newu ») et alléger les logs par connexion.
- [ ] Re-stress-tester : `siege`/`ab`, connexions parallèles + keep-alive, vérifier 0 fuite de fd (`ls /proc/PID/fd`).

## Étape 5 — Config & fichiers du dépôt

- [ ] **`webserv2.conf`** utilise la syntaxe nginx `http { }` / `events { }` que le parser **ne supporte pas** (il attend `server` au top-level) → soit l'aligner sur le format réel, soit le supprimer.
- [ ] **`conf/default.conf`** : `root /var/www/...` doit exister sur la machine d'éval → fournir un `www/` de démo dans le dépôt + config pointant dessus.
- [ ] Fournir une config qui **démontre chaque feature** (autoindex, redirect, upload, CGId, error_page, multi-port) — exigé par le sujet pour la défense.
- [ ] Finir le **Validator** (voir tes notes dans le `TODO.md` racine) : cgi_pass, upload_store cohérent, error_page absolu, doublons listen cross-server (déjà partiellement fait).

## Étape 6 — Qualité / nettoyage

- [ ] **B12** — remplacer `strtoull` (C99) par `strtoul` + contrôle overflow (portabilité macOS/clang).
- [ ] Factoriser `itos`/`toString` (3 implémentations, risque ODR).
- [ ] `mime_types` en `static const` local ; casts C++ ; prédicats `bool`.
- [ ] `Makefile` : ajouter `-MMD -MP` (dépendances headers).
- [ ] Retirer du dépôt évalué ce qui n'est pas demandé (`webserv-studies/`, `*.md` internes, `test.html`) ou les ranger.

## Étape 7 — README (chapitre V, obligatoire)

- [ ] Réécrire `README.md` en **anglais** avec : 1ʳᵉ ligne en italique `*This project has been created as part of the 42 curriculum by ninisse, endrit.*`, sections **Description**, **Instructions** (build/run), **Resources** (références + **comment l'IA a été utilisée**, pour quelles tâches).

## Étape 8 — Bonus (seulement une fois le mandatory nickel)

Voir [`BONUS.md`](BONUS.md) : cookies/sessions, multi-CGI, et la **suite de tests Python** à fournir (le sujet recommande de tester dans un langage adapté — Python).

---

## Checklist « prêt pour la défense »

- [ ] `make re` sans warning, pas de relink inutile.
- [ ] Aucune requête ne hang (CGI, keep-alive, body incomplet).
- [ ] Path traversal impossible (lecture ET écriture).
- [ ] CGI GET/POST fonctionnel + timeout.
- [ ] Stress test soutenu, 0 fuite de fd/mémoire (`valgrind` propre au ctrl-C).
- [ ] Config de démo couvrant toutes les features.
- [ ] README conforme.
- [ ] Chaque membre sait **expliquer** chaque partie (règle IA du sujet).
