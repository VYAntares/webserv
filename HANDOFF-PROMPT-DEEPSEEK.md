# PROMPT DE PASSATION — Adaptation des notes d'étude webserv au code réel

> Copie tout ce fichier comme prompt. Il contient : la mission, l'état exact du travail,
> la méthode à suivre, les règles de style non négociables, une fiche technique complète
> du code (pour éviter de tout relire), et la checklist précise de ce qui reste à faire.

---

## 1. LA MISSION

Tu travailles sur le dépôt `/home/antares/Documents/webserv` — un serveur HTTP en C++98
(projet 42). Le dossier `www/webserv-studies/` contient des notes d'étude HTML très
soignées (une "mindmap" centrale + des fiches par domaine). Ces pages ont été écrites
comme des tutoriels génériques ; la mission est de les **adapter au code réel du projet**.

**Règle d'or n°1 : tu ne modifies QUE des fichiers `.html` dans `www/webserv-studies/`.**
Jamais le code C++, jamais les .conf, jamais rien d'autre.

**Règle d'or n°2 : tout extrait de code montré dans une page doit correspondre au vrai
code du projet.** Pas de pseudo-code, pas de `Connection conn`, pas de `build_response()`
inventé, pas de C++11 (`auto`, range-for, `nullptr`, `std::to_string` sont INTERDITS —
le projet est en C++98). Quand une page montre du code, c'est le code de
`srcs/` / `includes/`, copié fidèlement (commentaires français du code inclus), avec le
chemin réel du fichier dans le bandeau du panel (ex. `srcs/http/HttpParser.cpp — treatHeader()`).

**Règle d'or n°3 : tu ne casses pas le design.** Les pages partagent
`assets/webserv-shared.css`. Tu réutilises les classes existantes à l'identique,
tu ne crées pas de nouveau CSS, tu ne touches pas aux `<script>` de bas de page
(sauf si tu ajoutes une section avec ancre → ajoute l'id dans le tableau `navIds`).

---

## 2. CE QUI EST DÉJÀ FAIT (ne pas y retoucher)

- `05-CGI/` : 4 nouvelles fiches créées (cgi-mecanisme, cgi-env, cgi-pieges, cgi-event-loop).
- `06-robustesse/` : 4 nouvelles fiches créées (gestion-erreurs, timeouts-deconnexions,
  limites-protections, stress-tests-outils).
- `07-architecture/full-architecture.html` : réécrite entièrement (pattern Reactor réel).
- `webserv-mindmap.html` : sections 05 (CGI) et 06 (Robustesse) réécrites, cartes cliquables
  (`data-page="..."` + `<a href>` dans le `<h3>`, comme les cartes des sections 01-04).
- `01-sockets-tcp/` : toutes les pages adaptées (socket-lifecycle, multi-port,
  network-addressing, recv-send, fcntl-nonblocking + notes dans les pages internals).
- `02-io-multiplexing/` : toutes les pages adaptées (event-loop, epoll, select-poll-epoll,
  epoll-wait-before-io).
- `03-http-protocol/` : `mime-types.html` et `multipart-upload.html` adaptées.

Ouvre une de ces pages adaptées (ex. `02-io-multiplexing/event-loop.html` ou
`05-CGI/cgi-pieges.html`) pour voir EXACTEMENT le style attendu avant d'écrire quoi que ce soit.

---

## 3. CE QU'IL RESTE À FAIRE (ta checklist)

### A. Finir `03-http-protocol/` (4 pages)

1. **`chunked-encoding.html`** (11 code-panels) — la plus importante. Remplace le
   décodeur chunked générique par le vrai : `HttpParser::readChunked()` et
   `endOfChunked()` (copie-les depuis `srcs/http/HttpParser.cpp`, ils sont dans la
   fiche technique §6 ci-dessous). Points à mettre en avant :
   - état `R_CHUNKED` déclenché par `findHeaders()` sur la CLÉ `Transfer-Encoding`
     (bug corrigé : avant, un header `X-Foo: chunked` déclenchait le mode) ;
   - taille hexa lue avec `strtol(..., 16)`, tolérance chunk-extensions (`;`) ;
   - comptage `_bodyReceived` UNIQUEMENT quand le chunk est entier dans le buffer
     (bug corrigé : re-comptage → faux 413) ;
   - 413 par location, body drainé ; le CGI reçoit un body déjà dé-chunké + EOF.
2. **`http-requete.html`** — remplace le parsing générique par la machine à états
   `HttpParser` : `runParsing()` (boucle do/while), `treatHeader()` (\r\n\r\n, 431 à
   32 Ko), `headerParser()` (première ligne + map headers + résolution de la limite
   body par location AVANT le body), `checkFirstLine()` (méthode en majuscules, URI
   commençant par `/`, décodage %XX, version HTTP/1.1|1.0).
3. **`http-methods.html`** — ancre sur le vrai routing : `methodImplemented()`
   (GET/POST/DELETE/HEAD, sinon 501), `methodAllowed()` (allowed_methods de la
   location, sinon 405, liste vide = tout autorisé), `StaticHandler::handleGet/
   handlePost/handleDelete` (GET→ifstream, POST→ofstream+201 si création+decodeHexa
   des formulaires urlencoded, DELETE→unlink→204/404), HEAD = `stripBody()` +
   keepAlive false.
4. **`http-response.html`** et **`http-response-in-style-of-07-architecture.html`** —
   remplace tout `build_response()` générique par `ARequestHandler::buildResponse()`
   réel (status line + Location/Content-Type/Set-Cookie conditionnels + Content-Length
   + Connection keep-alive/close) ; la décision keep-alive est dans
   `ClientHandler::_handleComplete()` (header Connection explicite sinon défaut par
   version HTTP). Codes gérés : voir `getReason()` dans la fiche §8. NB : la deuxième
   page est une variante visuelle de la première (style "architecture") — adapte les
   deux, même contenu factuel.

### B. `04-Fichier-de-configuration/` (7 pages)

Fichiers : `nginx-config-foundations.html`, `config-file-complete-guide.html`,
`config-terms-glossary.html`, `config-required-minimum.html`,
`routing-longest-prefix.html`, `lexer-parser-validation.html`, `root-to-file.html`.

Pour chacune : lis-la, repère les extraits de code/config génériques, remplace par le
réel. Les gros morceaux :
- **`lexer-parser-validation.html`** : montrer les vraies classes — `ConfigLoader`
  (argc, extension .conf, stat S_ISREG, fichier non vide) → `Lexer` (5 tokens :
  TOK_WORD/LBRACE/RBRACE/SEMI/EOF, quotes, commentaires #, line/col pour les erreurs)
  → `Parser` (parseServerBlock/parseLocationBlock/putDirective + erreurs "missing ';'",
  duplicats, directives location-only vs server-only) → `Validator`
  (propogateServerToLocations, checkLocationConfig, checkDuplicateIntraServer).
  Remplace tout struct `ServerConfig`/`LocationConfig` générique par les vraies structs
  (§5 de la fiche).
- **`routing-longest-prefix.html`** : le vrai algo est `findLocation()` (utils.cpp) —
  réduction de l'URI segment par segment (`rfind('/')` + `erase`) jusqu'à match EXACT
  avec un `location.path`, partagé entre Router et HttpParser (limite body par location
  dès les headers). Le Router matche sur le chemin SANS query string (bug corrigé :
  `/upload?x=1` ne matchait pas `/upload`).
- **`root-to-file.html`** : `Router::resolvePath()` (root + URI → normalizePath →
  gestion du slash final → index si répertoire) + l'ordre complet du Router (fiche §7)
  + `StaticHandler` (dossier sans slash → 301, autoindex → throwList, sinon 403).
- **`config-required-minimum.html` / glossary / complete-guide / nginx-foundations`** :
  vérifier que chaque directive citée existe avec la bonne syntaxe (§4), utiliser des
  exemples tirés de notre vrai `conf/default.conf` (deux serveurs : 8080 avec le
  tester 42 + suite maison, 8079 site2 ; error_page pour 400/403/404/405/413/431/500/
  501/502/504 ; `cgi_pass .py /usr/bin/python3` et `.bla → cgi_test`, `upload_store
  ./www/upload`, `client_max_body_size` 10m/100/1g selon la location).

### C. La mindmap `webserv-mindmap.html` (le reste)

- **Section 00 (vision)** : le flow "cycle de vie" peut citer nos vraies étapes
  (epoll_wait → handle_input → HttpParser → Router → handler → handle_output).
  La checklist des contraintes est déjà correcte — vérifie juste.
- **Sections 01 à 04** : les textes des cartes sont corrects mais génériques ;
  injecte les noms réels là où c'est naturel (ServerHandler, EventLoop, HttpParser,
  Lexer/Parser/Validator, findLocation...). Ne casse pas les `data-page` existants.
- **Hero stats** : « 6 Domaines clés » → il y en a maintenant 8 sections numérotées
  (00-07) ; ajuste si pertinent (ex. "8 domaines", "30+ fiches").
- **Section 08 ressources & bonus** : vérifier la cohérence (le bonus cookies/sessions
  EST implémenté chez nous : Set-Cookie via parseCgi + setCookie, scripts cookies.py
  et session.py — adapte le texte du bonus pour le dire).

---

## 4. LA MÉTHODE (fais exactement comme ça)

1. **Lis d'abord le code réel** concerné par la page (chemins en §5-§8 ; tout est déjà
   résumé dans la fiche, ne fais confiance qu'à elle ou au code lui-même, jamais au
   contenu générique de la page).
2. **Lis la page entière** avant d'éditer (elles font 600-1100 lignes).
3. **Édits ciblés**, pas de réécriture totale : le texte pédagogique, les analogies,
   les schémas restent. Tu remplaces :
   - les code-panels génériques → vrai code, bandeau = chemin réel du fichier ;
   - les affirmations fausses pour notre projet (ex. "timeout -1", "boucler jusqu'à
     EAGAIN", "backlog 10", `inet_ntoa`, `shutdown()`, `setenv()`...) → la réalité ;
   - et tu AJOUTES des encarts « Dans notre webserv » (info-box 🏗️ ou key-point)
     qui font le pont entre le concept enseigné et notre implémentation.
4. **Quand un pattern enseigné contredit le projet** (drain-loop EAGAIN, errno après
   read/write, EPOLLET...), garde-le seulement s'il est clairement présenté comme
   contre-exemple/culture générale, et ajoute la version réelle à côté.
5. **Vérifie après chaque page** : `python3 -c "import html.parser; ..."` n'est pas
   nécessaire, mais fais au minimum un grep des balises : le nombre de `<section` doit
   correspondre aux `</section>`, et les `navIds` du script de bas de page doivent
   matcher les `id=` des sections si tu en ajoutes.

---

## 5. RÈGLES DE STYLE HTML (non négociables)

Deux familles de pages, deux jeux de classes pour le code :

**Famille "step-section"** (01, 02, 03, 05, 06) — coloration par spans :
```html
<div class="code-panel" style="--step-color: var(--accent-2)">
  <div class="code-header">
    <div class="code-header-left">
      <div class="code-dots"><div class="code-dot"></div><div class="code-dot"></div><div class="code-dot"></div></div>
      <span class="code-filename">srcs/http/HttpParser.cpp — readChunked()</span>
    </div>
    <span class="code-tag" style="color:var(--accent-2); background:rgba(74,224,255,0.08)">Notre code</span>
  </div>
  <pre>...</pre>
</div>
```
Spans dans `<pre>` : `kw` (mots-clés), `type` (types), `fn-name` (fonctions),
`var` (variables), `num`, `str`, `cmt` (commentaires), `macro` (MACROS/defines),
`punct` (ponctuation, utilisé surtout dans les pages 01). Lignes surlignées :
`<span class="highlight-line">…</span>` (ligne clé) et `<span class="highlight-new">…</span>`
(ajout/conclusion). Certaines pages 01 utilisent aussi `highlight-warn`/`highlight-bad`.

**Famille "chapter"** (07-architecture, mime-types, multipart) — plus simple :
`<div class="code-block"><div class="code-header">titre</div><div class="code-body">…</div></div>`
avec spans `kw`, `fn`, `cm`, `hl`, `str`, `accent`. Regarde la page pour savoir quelle
famille elle utilise avant d'écrire.

**Échappement obligatoire dans les `<pre>`** : `&` → `&amp;`, `<` → `&lt;`, `>` → `&gt;`.
(Ex. `std::map<str, str>` s'écrit `std::map&lt;str, str&gt;`.)

**Encarts** : `<div class="info-box"><span>💡</span><div>…</div></div>`,
`<div class="warning"><span>⚠️</span><div>…</div></div>`, panels
`<div class="panel" style="--step-color: var(--accent-N)"><div class="panel-label">…</div><p>…</p></div>`.
Les accents HTML : les vieilles pages utilisent des entités (`&eacute;`), les pages
récentes de l'UTF-8 brut — les deux marchent, reste cohérent avec la page que tu édites.

**Liens croisés** : relatifs (`../05-CGI/cgi-pieges.html`,
`../webserv-mindmap.html`). Signature en footer : `eahmeti`.

---

## 6. FICHE TECHNIQUE DU CODE — HTTP (pour les pages 03)

Pipeline : `HttpParser (stateful) → HttpRequest (data pure) → Router → ARequestHandler`.

**HttpParser** (`srcs/http/HttpParser.cpp`, header dans `includes/http/HttpParser.hpp`) :
- États : `R_HEADERS, R_BODY, R_CHUNKED, COMPLETE, ERROR`. `#define MAX_HEADER_SIZE 32768`.
- `runParsing(buffer)` : `_buffer += buffer` puis boucle `do { switch(_state) } while
  (état a changé && != COMPLETE/ERROR)` — car un seul recv peut livrer headers+body.
- `treatHeader()` : cherche `\r\n\r\n` ; sinon si `_buffer.size() > MAX_HEADER_SIZE`
  → `setError(431)`. Ensuite `headerParser()` puis état R_BODY/R_CHUNKED/COMPLETE.
- `headerParser()` : première ligne via `istringstream >> method >> uri >> version`,
  `checkFirstLine()`, boucle getline sur les headers (`": "` obligatoire sinon 400),
  puis `findLocation(uri sans query, *_server)` pour poser `_maxBodySize` de la bonne
  location, `findHeaders()`, et si `_bodyExcepted > _maxBodySize` → `setError(413)`.
- `checkFirstLine()` : méthode non vide et UNIQUEMENT des majuscules sinon 400 ;
  URI commençant par `/` sinon 400 ; si `isEncoded(uri)` → `decodeHexa(uri, 0)` ;
  version HTTP/1.1 ou HTTP/1.0 sinon 400.
- `findHeaders()` : Content-Length purement numérique sinon 400 (`strtoull`) ;
  `Transfer-Encoding` contenant "chunked" (test sur la CLÉ !) → `_state = R_CHUNKED` ;
  toute valeur contenant "multipart/form-data" → `setBoundary()` (`_req.isMultipart = true`).
- `treatBody()` : consomme `min(left, _buffer.size())` ; si `_errorCode == 413` le
  body est JETÉ (erase sans append) — on draine pour pouvoir répondre 413 proprement ;
  complet → `_req.body = _body`, multipart → `getMp()`, état COMPLETE/ERROR.
- `setError(code)` : pour 413 hors chunked, NE bascule PAS l'état tout de suite
  (on finit de drainer) ; sinon `_state = ERROR`.
- `readChunked()` : boucle `while(true)` : cherche `\r\n` ; taille = `strtol(hex)` ;
  `end` doit pointer sur `\0` ou `;` (chunk-extension tolérée) sinon 400 ; si size==0
  → `endOfChunked(pos)` ; `needed = pos+2+size+2` — si buffer incomplet return ;
  `_bodyReceived += size` APRÈS le check de complétude (sinon faux 413) ; si dépasse
  `_maxBodySize` → setError(413) ; append au body seulement si pas d'erreur.
- `endOfChunked()` : attend `pos+4` (le `0\r\n\r\n` complet), `_req.body = _body`,
  multipart → `getMp()`, COMPLETE/ERROR.
- `reset()` : keep-alive — tout remis à zéro, `_maxBodySize` revient au défaut du
  server (la limite d'une location ne doit pas fuiter sur la requête suivante).

**ClientHandler** (`srcs/handlers/ClientHandler.cpp`) : recv 4096 / UN par EPOLLIN ;
`_handleComplete()` décide keep-alive (header Connection sinon HTTP/1.1=keep-alive,
HTTP/1.0=close), route, HEAD → stripBody + close, CGI → NULL → masque epoll vide ;
`handle_output()` : send `MSG_NOSIGNAL` depuis `_sent`, fini → keep-alive `_reset()` +
re-parse du reliquat (pipelining !) ou return -1.

**ARequestHandler::buildResponse()** : fallback error_page (configurée → getErrorPage,
sinon page générée `<h1>code reason</h1>`), puis status line + `Location:` si redirect
+ `Content-Type:` si `_type` non vide + `Set-Cookie:` si cookie + `Content-Length:` +
`Connection: keep-alive|close`.

## 7. FICHE TECHNIQUE — ROUTER & HANDLERS (pages 03 et 04)

Ordre exact de `Router::route(req, server, peerAddr, sink)` :
1. `req.error != 200` → `ErrorHandler(server, req.error, "")`.
2. `uriPath = uri` sans query string. `findLocation` → pas de location → 404.
3. `methodImplemented` (GET/POST/DELETE/HEAD) sinon **501**.
4. `methodAllowed` (allowed_methods ; vide = tout permis) sinon **405**.
5. `loc->return_path.first != -1` → `StaticHandler(req, loc, "")` (redirect AVANT
   tout accès disque).
6. `resolvePath()` : `normalizePath(root+uri, root)` — vide (traversal) → **403** ;
   gestion slash final ; si finit par `/` → `+ index`, ou test isDir.
7. POST + `upload_store` non vide : multipart → path = upload_store ; sinon
   upload_store + basename de l'URI.
8. `fileExist` (stat ; POST exempté) sinon **404** ; `forbiddenAccess` (GET→R_OK,
   DELETE→W_OK, POST→access du dossier parent) sinon **403**.
9. `isCgi()` : extension (`rfind('.')`) présente dans `loc->cgi_pass` ET méthode
   GET/POST → `CGIHandler(...)` + **return NULL** (asynchrone).
10. POST multipart → `MultipartHandler`, sinon `StaticHandler`.

`StaticHandler(req, loc, path)` : return_path → handleReturn (301/302 + Location) ;
isDir && !POST : URI sans slash final → 301 vers uri+'/', autoindex==1 → throwList()
(opendir/readdir, liens `<a>`), sinon 403 ; GET → ifstream+getType ; POST → ofstream,
decodeHexa si formulaire urlencoded (+/%), 201 si création ; DELETE → unlink → 204/404.

`findLocation(uri, server)` (utils.cpp) : boucle — cherche un match EXACT de
`shorturi` parmi les `locations[i].path` (le plus long gagne) ; sinon coupe au dernier
`/` (`rfind`+`erase`) et recommence ; `""` → `"/"`. Utilisée par Router ET HttpParser.

`normalizePath(p, root)` : `letNormalize` segmente sur `/`, ignore `.`/vides, `..`
dépile ; résultat doit commencer par root normalisé + caractère suivant `/` sinon `""`.

## 8. FICHE TECHNIQUE — CONFIG (pages 04)

**Structures** (`includes/config/ConfigStruct.hpp`) :
```
BaseBlock { pair<int,str> return_path(-1,""); map<int,str> error_page;
            string root, index; size_t max_body_client(SIZE_MAX); int autoindex(-1); }
Location : BaseBlock { map<str,str> cgi_pass; vector<str> allowed_methods;
                       string upload_store, path; }
Server   : BaseBlock { vector<addrport> listen; vector<str> server_name;
                       vector<Location> locations; }
Config   { vector<Server> cfg; }        // addrport = pair<uint32_t,int>
```

**Directives acceptées** — communes (putDirective) : `root`, `index`, `autoindex on|off`,
`client_max_body_size N[k|m|g]`, `error_page <codes 400-599...> <path>`, `return <code>
[path]`. Server-only : `listen` (multi-valeurs ; formats `8080`, `host:port`, `host`
seul → port 8080, `*` → 0.0.0.0 ; port validé 1-65535 ; résolution getaddrinfo AF_INET ;
duplicats refusés), `server_name` (multi). Location-only : `upload_store`, `cgi_pass
<.ext> <interpreteur>`, `allowed_methods` (multi). Duplicats de root/index/autoindex/
client_max_body_size → erreur. Défaut max_body : 1 Mo (posé dans parseServerBlock si
resté SIZE_MAX). Toute directive finit par `;` sinon erreur avec numéro de ligne.

**Validator** : config non vide ; chaque server a `listen` et au moins une location ;
duplicats de listen ENTRE serveurs refusés ; propagation server→location (root, index,
max_body, autoindex, return, error_page si absents) ; chaque location a root OU return ;
allowed_methods ∈ {GET, POST, DELETE} sans doublon ; error_page path absolu (commence
par `/`) ; upload_store exige POST dans allowed_methods ; cgi_pass : extension commence
par `.`, interpréteur par `/`.

**Lexer** : tokens TOK_WORD/TOK_LBRACE/TOK_RBRACE/TOK_SEMI/TOK_EOF ; quotes simples et
doubles (readQuote, erreur si non fermée) ; `#` commentaire jusqu'à fin de ligne ;
line/col trackés pour les messages d'erreur.

**ConfigLoader** : `./webserv [conf]`, défaut `./conf/default.conf` ; argc > 2 → erreur ;
extension `.conf` obligatoire ; `stat` + `S_ISREG` (un dossier "truc.conf" est refusé) ;
fichier vide refusé.

## 9. FICHE TECHNIQUE — RAPPELS TRANSVERSES

- EventLoop = singleton, pattern Reactor (Schmidt 1995) ; `epoll_wait(_epfd, events,
  128, 500)` ; `checkTimeOut()` à chaque tour, `CLIENT_TIMEOUT = 30` s ; try/catch
  autour du dispatch (sacrifice du handler) ; `HandlerEntry{handler,type}` dans
  `ev.data.ptr` ; EventType en octal (ACCEPT=01, READ=02, WRITE=04...).
- Convention : handler retourne -1 → `remove_handler` + `delete` → destructeur ferme le fd.
- Signaux : SIGINT/SIGTERM/SIGQUIT → `g_stop` (volatile sig_atomic_t) ; SIGPIPE ignoré
  + MSG_NOSIGNAL. Jamais d'errno après read/write (interdit par le sujet).
- `getReason()` couvre : 200 OK, 201 Created, 204 No Content, 301 Moved Permanently,
  302 Found, 400, 403, 404, 405, 413 Payload Too Large, 431, 500, 501, 502, 504.
- CGI : `Router` → `CGIHandler` (orchestrateur) → `CGIProcess` (pipe×2, fork, chdir,
  execve) + `CGIWriteHandler` (body→stdin) + `CGIReadHandler` (stdout→parseCgi →
  `IResponseSink::onCgiDone`). exit≠0 → 500, signal → 502, muet 30 s → 504.
  Env : CONTENT_LENGTH (= body.size() déjà dé-chunké), REQUEST_METHOD, SCRIPT_FILENAME,
  SCRIPT_NAME, QUERY_STRING, PATH_INFO, REQUEST_URI (exigés par cgi_test), SERVER_NAME/
  PORT (du header Host), REMOTE_ADDR/PORT, HTTP_* (majuscules, `-`→`_`).
- `conf/default.conf` réel : serveur 8080 (locations `/`, `/post_body` [POST, 100o],
  `/directory` [YoupiBanane, index youpi.bad_extension, cgi_pass .bla cgi_test, 1g],
  `/files` [autoindex on], `/private`, `/upload` [GET POST DELETE, upload_store
  ./www/upload], `/tiny` [100o], `/cgi-bin` [.py python3, .sh bash], `/studies`,
  `/old-page` [return 301], `/redirect302` [return 302]) + serveur 8079 (site2).
- Pas de `inet_ntoa` (pas dans les fonctions autorisées → `_buildPeerStr` manuel),
  pas de `shutdown()` utilisé, backlog = SOMAXCONN, `MAX_EVENTS = 128`.

## 10. EXEMPLE DE TRANSFORMATION ATTENDUE (avant → après)

**Avant** (générique, à remplacer) :
```html
<span class="code-filename">read_loop.cpp</span>
...
<pre>while (true) {
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) break;
    raw_request.append(buf, n);
    if (raw_request.find("\r\n\r\n") != std::string::npos) break;
}</pre>
```

**Après** (réel, avec bandeau chemin + commentaire d'intention) :
```html
<span class="code-filename">srcs/handlers/ClientHandler.cpp — l'accumulation, version epoll</span>
...
<pre><span class="cmt">// Chez nous, PAS de while(recv) : l'accumulation est répartie sur
// plusieurs événements epoll. L'état qui survit, c'est le HttpParser.</span>
<span class="kw">int</span> <span class="fn-name">ClientHandler::handle_input</span>() { ... }</pre>
```

Et quand la page enseigne un concept juste mais générique, ajoute :
```html
<div class="info-box"><span>🏗️</span><div>
  <strong>Dans notre webserv :</strong> ... (nom de classe réel, fichier réel, valeur réelle)
</div></div>
```

## 11. VÉRIFICATION FINALE

Après chaque page : relis le diff, vérifie l'échappement HTML des `<pre>`, vérifie
qu'aucun identifiant inventé ne subsiste (grep `Connection\b|sendBuf|recvBuf|
build_response|ServerBlock|ConnInfo|HttpResponse::` doit rendre 0 dans la page).
À la fin : lancer le serveur (`make && ./webserv conf/default.conf`) et ouvrir
`http://localhost:8080/studies/` pour vérifier visuellement que rien n'est cassé.
