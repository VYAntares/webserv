# srcs/http/ — Résumé & TODO

## Rôle dans le projet

Ce dossier contient **toute la logique HTTP** : parser de requête, structure de requête, routeur, et les différents handlers de réponse (statique, CGI, erreur). C'est le **métier** du serveur — séparé du cycle de vie socket (handlers/) et du dispatch événementiel (core/).

Le pipeline est documenté en haut de [HttpParser.hpp](../../includes/http/HttpParser.hpp) :

```
HttpParser  →  HttpRequest  →  Router  →  IRequestHandler
(stateful,    (pure data,     (choisit    (Static, CGI,
 accumule,     méthode, uri,   le bon       Error...)
 parse)        headers, body)  handler)
```

### Découpage SOLID — l'idée centrale

Le `ClientHandler` n'instancie **jamais** un handler de réponse concret. Il demande au `Router` un `IRequestHandler*` et appelle `buildResponse()` dessus, sans savoir si c'est du statique, du CGI, ou une erreur. C'est exactement **DIP** : dépendance vers l'abstraction.

| Classe              | Responsabilité unique (SRP)                                                 |
| ------------------- | --------------------------------------------------------------------------- |
| `HttpParser`        | Octets bruts (recv buffer) → `HttpRequest` typée. State machine, accumule. |
| `HttpRequest`       | POD : method, uri, version, headers, body, error.                          |
| `Router`            | `(HttpRequest, Server config)` → bon `IRequestHandler*`. Pas d'I/O.        |
| `IRequestHandler`   | Interface : `buildResponse()` retourne la string HTTP complète.            |
| `StaticHandler`     | Sert un fichier depuis le disque, ou un listing autoindex.                 |
| `CGIHandler`        | Délègue à srcs/cgi/, parse la sortie CGI, renvoie une réponse HTTP.        |
| `ErrorHandler`      | Renvoie une page d'erreur (custom si config, défaut sinon).                |

Conséquence : ajouter un handler (`UploadHandler`, `RedirectHandler`, `ProxyHandler`) = nouvelle classe qui hérite de `IRequestHandler` + un `else if` dans `Router`. Rien d'autre ne bouge.

---

## État actuel

### `HttpRequest` ([HttpRequest.hpp](../../includes/http/HttpRequest.hpp))
**Fait, minimal mais suffisant.** Champs : `error`, `method`, `uri`, `version`, `body`, `headers`. POD, pas de logique. ✓.

### `IRequestHandler` ([IRequestHandler.hpp](../../includes/http/IRequestHandler.hpp))
**Fait, minimal.** Une seule méthode pure `buildResponse() = 0`, destructeur virtuel. Parfait pour SOLID/DIP.

### `HttpParser` ([HttpParser.cpp](HttpParser.cpp))
**Partiellement fait — couvre le cas GET et POST simples avec Content-Length.**
- State machine : `R_HEADERS → R_BODY → COMPLETE` (+ `ERROR`, `R_CHUNCKED` déclaré mais inutilisé).
- `runParsing` : accumule dans `_buffer`, cherche `\r\n\r\n`, sépare headers/body.
- `headerParser` : parse première ligne (method, uri, version), parse headers `clé: valeur`, lit `Content-Length`.
- `checkFirstLine` : vérifie method en majuscules ASCII, uri commence par `/`, version `HTTP/1.0` ou `HTTP/1.1`.
- Erreurs : 400 (mal formé), 413 (body > max_body_client).

### `Router` ([Router.cpp](Router.cpp))
**Squelette en place mais largement incomplet.**
- `route()` : check error parser → check best location match → check method implementée → renvoie `StaticHandler` (en dur).
- `bestRouteFound()` : longest-prefix match, mais l'algorithme actuel a un bug (cf. TODO 4).
- `methodImplemented()` : GET, POST, DELETE OK.
- `fileFound()`, `forbiddenAccess()` : déclarés mais **non utilisés** (appels commentés).
- Aucun appel à `methodAllowed()` (checker les `allowed_methods` de la location).
- Aucune logique CGI : `if (isCgi())` est commenté.
- Aucune logique return/redirect.

### `StaticHandler` ([StaticHandler.cpp](StaticHandler.cpp))
**Stub.** Retourne toujours `"static handler response"` en dur. Aucun accès disque, aucun autoindex, aucun mime type.

### `CGIHandler` ([CGIHandler.cpp](CGIHandler.cpp))
**Stub.** Retourne toujours `"CGI handler response"`. Voir [srcs/cgi/SUMMARY.md](../cgi/SUMMARY.md) pour le plan d'attaque.

### `ErrorHandler` ([ErrorHandler.cpp](ErrorHandler.cpp))
**Stub + bug compilation.**
- ⚠️ [ErrorHandler.cpp:18](ErrorHandler.cpp#L18) : `case` orphelin sans valeur ni body → **ne compile pas en l'état**. À corriger immédiatement.
- Retourne toujours `200 OK` (!) avec le body `"Error handler response" + reason`. Le status code dans la première ligne n'est jamais `_error`. C'est-à-dire que **toutes les erreurs renvoient 200 au client** aujourd'hui.

---

## TODO — points manquants

### 1. ❗ `ErrorHandler` ne compile pas
[ErrorHandler.cpp:18](ErrorHandler.cpp#L18) : `case` incomplet. Supprimer cette ligne (et le `\n` qui suit).

### 2. ❗ `ErrorHandler` renvoie un status 200 au lieu de l'erreur
[ErrorHandler.cpp:29](ErrorHandler.cpp#L29) : `"HTTP/1.1 200 OK\r\n"` est en dur. → utiliser `_error` + `getReason()` pour formatter la première ligne : `"HTTP/1.1 " << _error << " " << getReason() << "\r\n"`.

### 3. ❗ `ErrorHandler` n'utilise jamais les `error_page` custom du config
Le sujet exige : *"Set up default error pages."* Implicite : les custom de la config doivent être utilisées si présentes. Aujourd'hui `ErrorHandler` ne reçoit pas la `Location` en paramètre, donc il ne peut pas savoir.
→ Le `Router` doit passer la `Location*` au `ErrorHandler(code, loc)` pour qu'il puisse :
1. Si `loc.error_page.find(code) != end` → servir le fichier pointé.
2. Sinon → générer une page HTML par défaut (la fonction `getReason()` + un `<h1>` minimal).

### 4. ❗ `Router::bestRouteFound` a un bug logique
[Router.cpp:61-82](Router.cpp#L61-L82) : la condition `server.locations[i].path.find(shorturi) == 0 && server.locations[i].path == shorturi` n'est satisfaite que si `path == shorturi` exactement. La double condition est redondante et empêche le match préfixe.
→ Réécrire en :
```cpp
for (size_t i = 0; i < server.locations.size(); i++) {
    const std::string& p = server.locations[i].path;
    if (uri.compare(0, p.size(), p) == 0 && (int)p.size() > best_len) {
        loc = &server.locations[i];
        best_len = p.size();
    }
}
```
(et drop la boucle `while (shorturi.length() != 0)` qui ne sert plus à rien). Et accepter le match `/` comme fallback (toujours préfixe de tout).

### 5. ❗ `Router` ne check pas `allowed_methods` (405)
Une fois la location matchée, si `loc->allowed_methods` est non vide et ne contient pas `req.method` → renvoyer `405 Method Not Allowed` avec header `Allow: GET, POST` (les méthodes effectivement autorisées). Aujourd'hui c'est commenté ([Router.cpp:28-29](Router.cpp#L28-L29)).

L'ordre du `ideas.txt` racine est correct :
1. Route inexistante → 404
2. Méthode inconnue (pas GET/POST/DELETE) → 501
3. Méthode connue mais pas dans `allowed_methods` → 405

### 6. ❗ `Router` ne gère pas `return` (redirect)
Si `loc->return_path.first != -1`, court-circuiter tout le reste et renvoyer un nouveau handler `RedirectHandler` (ou une variante d'`ErrorHandler`) qui produit la réponse :
- Status `loc->return_path.first` (ex: 301, 302, 307...).
- Header `Location: <loc->return_path.second>` si non vide.
- Body court ou vide.

### 7. ❗ `Router` ne route jamais vers CGI
Voir [srcs/cgi/SUMMARY.md TODO 1](../cgi/SUMMARY.md).

### 8. ❗ `StaticHandler` ne sert aucun fichier
Tout est à écrire. Étapes :
- Recevoir `Location*` + `HttpRequest*` en paramètres ctor.
- Résoudre le chemin filesystem : `loc->root` + (uri privé du préfixe `loc->path`). Ex : `loc->path = "/files"`, `loc->root = "/var/www/shared"`, `req.uri = "/files/img/cat.png"` → chemin disque `/var/www/shared/img/cat.png`.
- `stat()` :
  - Si fichier → ouvrir, lire (read sur disque autorisé sans poll), MIME-typer selon extension, renvoyer 200 + Content-Type + Content-Length + body.
  - Si dossier :
    - Si `loc->index` non vide et `<dir>/<index>` existe → servir cet index.
    - Sinon si `loc->autoindex == 1` → générer HTML de listing avec `opendir/readdir/closedir`.
    - Sinon → 403.
  - Si introuvable → 404 (l'ErrorHandler s'en chargera : `StaticHandler` peut renvoyer une string vide + setter un flag, ou être détecté en amont par Router).
- Pour DELETE : `unlink()` le fichier, renvoyer 204 No Content (ou 200).
- Pour POST avec `upload_store` : écrire le body dans `<upload_store>/<nom>`. Gérer multipart/form-data si besoin (le sujet n'oblige pas mais les navigateurs envoient comme ça par défaut).

### 9. ❗ `HttpParser` ne gère pas `Transfer-Encoding: chunked`
[HttpParser.hpp:17](../../includes/http/HttpParser.hpp#L17) : `R_CHUNCKED` est déclaré mais jamais utilisé. Un POST avec body chunké (curl, browsers HTTP/1.1 sans Content-Length) ne fonctionne pas.

Le sujet exige : *"for chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the body."*

→ Après `headerParser`, si `headers["Transfer-Encoding"] == "chunked"` (insensible casse) → entrer dans `R_CHUNCKED` :
- Lire la taille en hex jusqu'à `\r\n`.
- Si taille = 0 → fin, attendre `\r\n\r\n` final (trailers ignorés).
- Sinon lire `size` octets, append au body, attendre `\r\n`, recommencer.

### 10. ❗ `HttpParser::reset` est incomplet
[HttpParser.cpp:103-110](HttpParser.cpp#L103-L110) : les `clear()` sont commentés. Sur keep-alive (deuxième requête), `_buffer` peut contenir du résidu de la première requête (pipelining HTTP) → le code est presque OK pour ça (le résidu est valide) — mais `_body` et `_header` traînent.
→ Décommenter `_body.clear()` et `_header.clear()` au minimum. Pour `_buffer`, garder le résidu post-fin-de-requête est en fait souhaitable pour pipelining ; à ne **pas** clear, mais bien tester.

### 11. ❗ `HttpParser` parse les headers de façon fragile
[HttpParser.cpp:74-77](HttpParser.cpp#L74-L77) : `sep = line.find(": ")` impose un espace après les deux-points. La RFC autorise zéro ou plusieurs espaces. Header `Host:localhost` (sans espace) → 400. Trop strict.
→ `sep = line.find(':')`, puis trim left les espaces sur la valeur.

### 12. `HttpParser` ne normalise pas les headers en lower-case
HTTP est case-insensitive sur les noms de header. `headers["content-length"]` et `headers["Content-Length"]` doivent être le même slot. Aujourd'hui c'est case-sensitive → `content-length` (curl envoie comme ça) sera ignoré.
→ Normaliser en lower-case à l'insertion, et lookup toujours en lower-case.

### 13. `HttpParser::checkFirstLine` rejette les versions futures sans nuance
Une requête `HTTP/2.0` reçoit 400. RFC veut 505 (HTTP Version Not Supported) — mais c'est mineur, 400 passe à la défense.

### 14. `Router::fileFound` / `forbiddenAccess` à supprimer ou intégrer
Ces deux fonctions sont déclarées et définies mais jamais appelées. Soit on les intègre dans `StaticHandler` (qui est le bon endroit pour vérifier l'accès disque), soit on les supprime.

### 15. `Router` retourne raw `new`, owner ambigu
Le `ClientHandler` reçoit un `IRequestHandler*` et est censé le `delete` — voir TODO 1 de [srcs/handlers/SUMMARY.md](../handlers/SUMMARY.md). À documenter dans la docstring de `Router::route()` : "ownership transferred to caller".

---

## Priorisation

| Priorité  | Tâche                                                                                       |
| --------- | ------------------------------------------------------------------------------------------- |
| Critique  | 1 (ErrorHandler ne compile pas), 2/3 (ErrorHandler mauvais code/sans custom), 4 (Router bug), 5 (allowed_methods), 8 (StaticHandler vide) |
| Important | 6 (return/redirect), 7 (CGI routing), 9 (chunked), 10 (reset), 11/12 (headers fragiles)     |
| Utile     | 13 (HTTP/2 → 505), 14 (cleanup), 15 (doc ownership)                                         |

**État global du dossier :** l'architecture est correctement posée (Router + IRequestHandler), mais **aucun handler ne produit de vraie réponse**. La priorité absolue : (a) débloquer la compilation (TODO 1), (b) implémenter `StaticHandler` pour de vrai (TODO 8) — sans ça, le serveur ne peut pas servir un seul fichier statique, ce qui est la première exigence du sujet (*"You must be able to serve a fully static website."*).
