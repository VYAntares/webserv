# Rapport — Passage du tester 42 (`./tester`) et du CGI (`cgi_test`)

Date : 2026-07-09
Résultat final : **les 24 tests du tester passent, 0 erreur, exit 0** (y compris les
tests de charge : 20 workers × 5000 GET, 128 workers × 50 GET, 20 workers × 5 POST de 100MB).

## À propos de `./cgi_tester`

`./cgi_tester` (racine du repo) et `www/cgi-bin/cgi_test` sont **le même binaire**
(md5 identiques : `fab00e9d...`). Ce n'est pas un second testeur : c'est l'exécutable
CGI que le tester exerce via les POST sur `.bla`. Il est donc entièrement couvert
par le run du tester.

## Configuration utilisée (`conf/tester.conf`)

- `/` : GET uniquement, autoindex on
- `/post_body` : POST, `client_max_body_size 100`
- `/directory` : GET + POST, index `youpi.bad_extension`, `cgi_pass .bla → cgi_test`,
  **`client_max_body_size 1g` ajouté** (sinon héritage du `1m` serveur → 413 sur le
  POST 100MB du tester)
- **Symlink créé : `www/directory → www/YoupiBanane`** — le root est de sémantique
  nginx (`root + uri`), donc `/directory/...` cherche dans `www/directory/...`.
  Le symlink fait pointer ça vers YoupiBanane comme exigé. `normalizePath()` étant
  purement textuel, le symlink ne déclenche pas le garde-fou anti-`../`.

## Bugs trouvés et corrigés

### 1. `PATH_INFO` et `REQUEST_URI` absents de l'environnement CGI
**Fichier : `srcs/cgi/CGIProcess.cpp` (`addUri`)**
`cgi_test` exige `PATH_INFO` et vérifie sa cohérence avec `REQUEST_URI` :
sans elles il répond `500 "PATH_INFO not found"` / `"PATH_INFO incorrect"`.
→ Ajout de `PATH_INFO` (chemin de l'URI sans query) et `REQUEST_URI` (URI complète).
Diagnostic fait en exécutant `cgi_test` à la main avec des environnements bisectés.

### 2. `client_max_body_size` de la location jamais appliqué
**Fichiers : `HttpParser.{hpp,cpp}`, `Router.cpp`, `utils.{hpp,cpp}`, `ClientHandler.cpp`**
Le parser était construit avec la limite du **serveur** uniquement ; la limite
par location n'était vérifiée nulle part. Conséquences : le POST 100MB vers le CGI
prenait 413 (limite serveur 1m), et un POST > 100 octets sur `/post_body` serait
passé si la limite serveur avait été plus grande.
→ La logique de matching de location (`bestRouteFound`) a été extraite dans
`findLocation()` (utils, partagée avec le Router, évite l'include circulaire
Router↔ClientHandler↔HttpParser). Le parser résout la location dès la fin des
headers et applique **sa** limite avant de lire le body. `reset()` restaure la
limite serveur pour la requête keep-alive suivante.

### 3. Chunked : chunks comptés plusieurs fois → faux 413
**Fichier : `srcs/http/HttpParser.cpp` (`readChunked`)**
`_bodyReceived += size` était fait **avant** de vérifier que le chunk était entier
dans le buffer. Un chunk arrivant en plusieurs `recv()` (buffer de 4k, chunks de
64k du client Go) était re-compté à chaque appel (~16×) : un body chunked de
~67MB dépassait fictivement 1g → 413. C'est pour ça que le POST 100MB échouait
via le tester (Go envoie en chunked) mais passait via curl (Content-Length).
→ L'incrément et le check 413 sont déplacés après le test de complétude du chunk.
Diagnostic : bisection de tailles (seuil ~64MB incohérent avec les limites
configurées) + trace `setError` montrant `received=1073807360` pour 100MB envoyés.

### 4. Erreur 413 écrasée en mode chunked
**Fichier : `srcs/http/HttpParser.cpp` (`runParsing`)**
`runParsing` faisait `_req.error = 200;` à chaque appel. En chunked, le 413 est
flaggé sans changer l'état (pour drainer le body) ; si le terminateur `0\r\n\r\n`
arrivait dans un `recv()` ultérieur, l'erreur était réinitialisée et la requête
traitée normalement : `POST /post_body` chunked de 200 octets répondait 200 au
lieu de 413.
→ `_req.error = (_errorCode != 0) ? _errorCode : 200;`

## Vérifications de non-régression (manuelles, après corrections)

| Test | Attendu | Obtenu |
|---|---|---|
| HEAD / (GET only) | 405 | 405 |
| POST / , DELETE / | 405 | 405 |
| PUT / | 501 | 501 |
| GET /post_body | 405 | 405 |
| POST /post_body 50 o | 2xx | 201 |
| POST /post_body 101 o (CL) | 413 | 413 |
| POST /post_body 200 o (chunked) | 413 | 413 |
| GET /directory/youpi.bla (statique) | 200 | 200 |
| POST .bla chunked 1k / 2M / 100M | 200 | 200 |
| POST .bla Content-Length 100MB | 200 (1,4 s) | 200 |
| Upload multipart (`default.conf`, port 7001) | 201 + fichier créé | 201 |

Note `default.conf` : les 404/500 observés viennent des chemins `/var/www/...`
inexistants sur cette machine (et `./uploads` manquant, créé depuis) — pas des
corrections ci-dessus.

## Reproduire

```sh
make
ln -sfn YoupiBanane www/directory        # déjà en place
./webserv conf/tester.conf &
./tester http://localhost:8080           # appuyer sur Entrée aux prompts
```


Voici le strict minimum modifié dans cette session pour faire passer le tester à 100% :

  Code — 7 fichiers

  1. srcs/cgi/CGIProcess.cpp (le fix CGI)
  Dans addUri() : ajout de 2 variables d'environnement que cgi_test exige :
  envp->push_back("PATH_INFO=" + scriptPath);
  envp->push_back("REQUEST_URI=" + _req.uri);

  2. srcs/http/HttpParser.cpp (3 fixes)
  - headerParser() : résolution de la location dès les headers pour appliquer son client_max_body_size (avant, seule la limite serveur existait) :
  const Location* loc = findLocation(_req.uri.substr(0, _req.uri.find('?')), *_server);
  if (loc) _maxBodySize = loc->max_body_client;
  - readChunked() : _bodyReceived += size déplacé après le check de complétude du chunk (avant, un chunk incomplet était re-compté à chaque recv() → faux 413 sur le POST 100MB chunked du tester)
  - runParsing() : _req.error = (_errorCode != 0) ? _errorCode : 200; au lieu de _req.error = 200; (le 413 chunked était écrasé → /post_body 200 octets répondait 200 au lieu de 413)
  - reset() : restaure _maxBodySize = _server->max_body_client

  3. includes/http/HttpParser.hpp — constructeur HttpParser(const Server&) + membre const Server* _server

  4. srcs/utils/utils.cpp — nouvelle fonction findLocation() (le longest-prefix match extrait du Router, partagé avec le parser)

  5. includes/utils/utils.hpp — déclaration de findLocation + include de ConfigStruct.hpp

  6. srcs/http/Router.cpp — bestRouteFound() délègue à findLocation() (déduplication)

  7. srcs/handlers/ClientHandler.cpp — une ligne : _parser(server) au lieu de _parser(server.max_body_client)

  Setup — 2 changements (pas du code)

  - conf/tester.conf : client_max_body_size 1g; ajouté dans location /directory (sinon héritage du 1m serveur → 413 sur le POST 100MB)
  - Symlink : ln -sfn YoupiBanane www/directory (ton root est nginx-style root + uri, donc /directory/... cherche dans www/directory/...)

  En résumé conceptuel, il n'y a que 4 vrais bugs corrigés : les env vars CGI manquantes, la limite de body par location ignorée, le double comptage des chunks, et l'erreur 413 écrasée. Le reste (utils, Router,
  ClientHandler, hpp) n'est que la plomberie pour donner au parser l'accès à la config des locations.
