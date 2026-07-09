# Rapport de Diff - webserv vs webservPerfect

**Date:** 2026-07-09 23:03:44

> Les lignes vides sont ignorées dans les comparaisons (option `diff -B`).

## Résumé

| Statut | Nombre |
|--------|--------|
| Fichiers identiques (hors lignes vides) | 32 |
| Fichiers avec différences | 13 |
| Fichiers uniques à webserv | 0 |
| Fichiers uniques à webservPerfect | 0 |
| **Total** | **45** |

### Fichiers avec différences

     1. includes/handlers/CGIReadHandler.hpp
     2. includes/http/ARequestHandler.hpp
     3. includes/http/HttpParser.hpp
     4. includes/utils/utils.hpp
     5. srcs/handlers/CGIReadHandler.cpp
     6. srcs/handlers/ClientHandler.cpp
     7. srcs/handlers/ServerHandler.cpp
     8. srcs/http/ARequestHandler.cpp
     9. srcs/http/HttpParser.cpp
    10. srcs/http/MultipartHandler.cpp
    11. srcs/http/Router.cpp
    12. srcs/http/StaticHandler.cpp
    13. srcs/utils/utils.cpp

### Fichiers identiques (hors lignes vides)
Tous les autres fichiers (32) n'ont aucune différence autre que les lignes vides ajoutées dans webserv.

---

### `includes/handlers/CGIReadHandler.hpp`

```diff
11,12c11,12
< 		int 			getFd()		 const { return _fd; }
< 		int 			handle_input();
---
> 		int				getFd()		 const { return _fd; }
> 		int				handle_input();
16c16
< 		
---
> 
21a22,24
> 		void			reapChild(int& status);
> 		std::string		buildCgiResponse();
> 
26a30,31
> 		bool			_reaped;    // waitpid déjà fait (ne pas re-kill un pid recyclé)
> 		bool			_responded; // une réponse a été envoyée au sink
```

### `includes/http/ARequestHandler.hpp`

```diff
7c7,8
< 		
---
> 		ARequestHandler() : _ncode(200), _keepAlive(false), _noBody(false) {}
> 
14a16,17
> 		// keep-alive décidé par ClientHandler ; le header "Connection:" émis
> 		// par buildResponse doit refléter ce que le serveur fera vraiment.
15a19,21
> 		void				stripBody() { _body.clear(); _noBody = true; }
> 		// utilisé par CGIReadHandler pour propager le Content-Type du script
> 		void				setType(const std::string& t) { _type = t; }
21a28
> 		bool				_noBody;
```

### `includes/http/HttpParser.hpp`

```diff
15a16,18
> // taille max de la section headers d'une requête (protection mémoire)
> #define MAX_HEADER_SIZE 32768
> 
26c29
< 		explicit HttpParser(size_t maxBodySize);
---
> 		explicit HttpParser(const Server& server);
52a56,58
> 		// pour retrouver la limite de body de la location matchée par l'URI
> 		// (le parser lit le body AVANT le passage dans le Router)
> 		const Server*					_server;
```

### `includes/http/HttpRequest.hpp`

**Aucune différence** (hors lignes vides)

### `includes/http/MultipartHandler.hpp`

**Aucune différence** (hors lignes vides)

### `includes/http/MultipartParser.hpp`

**Aucune différence** (hors lignes vides)

### `includes/http/Router.hpp`

**Aucune différence** (hors lignes vides)

### `includes/http/StaticHandler.hpp`

**Aucune différence** (hors lignes vides)

### `includes/utils/utils.hpp`

```diff
1c1
< #pragma once 
---
> #pragma once
8a9,14
> 
> #include "../config/ConfigStruct.hpp"
> 
> // longest-prefix match de l'URI contre les locations du serveur
> // (partagé entre Router et HttpParser pour la limite de body par location)
> const Location*                     findLocation(const std::string& uri, const Server& server);
```

### `srcs/cgi/CGIProcess.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/cgi/SUMMARY.md`

**Aucune différence** (hors lignes vides)

### `srcs/config/ConfigLoader.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/config/Lexer.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/config/Parser.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/config/Validator.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/core/EventLoop.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/handlers/CGIHandler.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/handlers/CGIReadHandler.cpp`

```diff
3a4
> #include "../../includes/utils/utils.hpp"
4a6
> #include <cstdlib>
6a9
> #include <signal.h>
8a12,15
> CGIReadHandler::CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink)
> 							: _fd(fd), _pid(pid), _loc(loc), _sink(sink),
> 							  _reaped(false), _responded(false), _lastActivity(time(NULL)) {
> }
10,12c17,28
< 
< CGIReadHandler::CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink) 
< 							: _fd(fd), _pid(pid), _loc(loc), _sink(sink) {
---
> // Récupère le statut de l'enfant sans jamais bloquer la boucle : si le CGI a
> // fermé son stdout mais tourne encore (ex: boucle infinie après le print),
> // l'ancien waitpid bloquant gelait TOUT le serveur. On le tue proprement et
> // on sert quand même la sortie déjà reçue.
> void CGIReadHandler::reapChild(int& status) {
> 	pid_t r = waitpid(_pid, &status, WNOHANG);
> 	if (r == 0) {
> 		kill(_pid, SIGKILL);
> 		waitpid(_pid, &status, 0);
> 		status = 0; // la sortie reçue est complète : on la considère valide
> 	}
> 	_reaped = true;
14a31,68
> // La sortie d'un CGI n'est pas une réponse HTTP : c'est un bloc de headers
> // CGI (Status:, Content-Type:, ...) suivi d'une ligne vide puis du body
> // (RFC 3875). Avant, la sortie brute était collée telle quelle dans le body
> // de la réponse → les headers du script s'affichaient dans la page.
> std::string CGIReadHandler::buildCgiResponse() {
> 	int			code = 200;
> 	std::string	type;
> 	std::string	body;
> 
> 	size_t sep	= _out.find("\r\n\r\n");
> 	size_t skip	= 4;
> 	if (sep == std::string::npos) { // certains scripts n'émettent que du \n
> 		sep  = _out.find("\n\n");
> 		skip = 2;
> 	}
> 	if (sep == std::string::npos) // pas de bloc header : tout est body
> 		body = _out;
> 	else {
> 		std::string headers = _out.substr(0, sep);
> 		body = _out.substr(sep + skip);
> 
> 		std::istringstream iss(headers);
> 		std::string line;
> 		while (std::getline(iss, line)) {
> 			if (!line.empty() && line[line.size() - 1] == '\r')
> 				line.erase(line.size() - 1);
> 			if (line.compare(0, 7, "Status:") == 0)
> 				code = std::atoi(line.c_str() + 7);
> 			else if (line.compare(0, 13, "Content-Type:") == 0) {
> 				size_t v = 13;
> 				while (v < line.size() && line[v] == ' ')
> 					v++;
> 				type = line.substr(v);
> 			}
> 		}
> 	}
> 	if (code <= 0)
> 		code = 200;
15a70,74
> 	StaticHandler s(*_loc, code, body);
> 	if (!type.empty())
> 		s.setType(type);
> 	return s.buildResponse();
> }
17a77,78
> 	_lastActivity = time(NULL);
> 
26,28c87,92
< 		ErrorHandler err(*_loc, 502);
< 		_sink->onCgiDone(err.buildResponse());	// ne pas laisser le client sans réponse
< 		return -1;
---
> 		if (_sink) {
> 			ErrorHandler err(*_loc, 502);
> 			_sink->onCgiDone(err.buildResponse());	// ne pas laisser le client sans réponse
> 			_responded = true;
> 		}
> 		return -1;								// l'enfant sera tué/reapé par le destructeur
33c97,101
< 	waitpid(_pid, &status, 0);
---
> 	reapChild(status);
> 
> 	// client déjà déconnecté : _loc pointe dans son Server, ne plus y toucher
> 	if (!_sink)
> 		return -1;
35,39c103,111
< 	ARequestHandler* a = NULL;
< 	if (WIFSIGNALED(status))
< 			a = new ErrorHandler(*_loc, 502);
< 	else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
< 			a = new ErrorHandler(*_loc, 500, _out);
---
> 	std::string raw;
> 	if (WIFSIGNALED(status)) {
> 		ErrorHandler err(*_loc, 502);
> 		raw = err.buildResponse();
> 	}
> 	else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
> 		ErrorHandler err(*_loc, 500, _out);
> 		raw = err.buildResponse();
> 	}
41c113
< 			a = new StaticHandler(*_loc, 200, _out);
---
> 		raw = buildCgiResponse();
43,45c115,118
< 	std::string raw = a->buildResponse();
< 	delete a;
< 	_sink->onCgiDone(raw);
---
> 	if (_sink) {								// NULL si le client a déjà disparu
> 		_sink->onCgiDone(raw);
> 		_responded = true;
> 	}
49,51c122,137
< 
< 
< CGIReadHandler::~CGIReadHandler() { close(_fd); }
---
> // Trois responsabilités à la destruction (EOF normal, timeout, arrêt serveur) :
> // 1. fermer notre bout du pipe
> // 2. tuer + reaper l'enfant s'il ne l'a pas déjà été (pas de zombie)
> // 3. si aucun client n'a encore reçu de réponse (timeout CGI), envoyer 504 —
> //    "une requête ne doit jamais rester en attente indéfiniment" (sujet)
> CGIReadHandler::~CGIReadHandler() {
> 	close(_fd);
> 	if (!_reaped) {
> 		kill(_pid, SIGKILL);
> 		waitpid(_pid, NULL, 0);
> 	}
> 	if (_sink && !_responded) {
> 		ErrorHandler err(*_loc, 504);
> 		_sink->onCgiDone(err.buildResponse());
> 	}
> }
```

### `srcs/handlers/CGIWriteHandler.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/handlers/ClientHandler.cpp`

```diff
45c45
< 											_parser(server.max_body_client), _rh(NULL),
---
> 											_parser(server), _rh(NULL),
90a91,94
> 	if (req.method == "HEAD") {
> 		_rh->stripBody();
> 		_keepAlive = false;
> 	}
104a109,112
> 	std::cout << "[client " << _peerAddr << "] "
> 	          << req.method << " " << req.uri
> 	          << " (error " << req.error << ")\n";
> 
107a116,120
> 
> 	// Pour 413, on dit au client qu'on ne lira plus rien : shutdown(SHUT_RD)
> 	// évite un RST intempestif si le client continue d'envoyer son body.
> 	if (req.error == 413)
> 		shutdown(_fd, SHUT_RD);
```

< 	
---
> 
```

### `srcs/http/ARequestHandler.cpp`

```diff
14,15c12,18
< 		_body = "<html><body><h1>Redirecting to " + return_path.second + "</html></body></h1>";
< 		_location = return_path.second;
---
> 		// Le header Location n'a de sens que pour une redirection (3xx).
> 		// Pour "return 500 texte", le texte ne sert que dans le body.
> 		if (_ncode >= 300 && _ncode < 400) {
> 			_location = return_path.second;
> 			_body = "<html><body><h1>Redirecting to " + return_path.second + "</h1></body></html>";
> 		} else
> 			_body = "<html><body><h1>" + return_path.second + "</h1></body></html>";
18c21
< 		_body = "<html><body><h1>Redirecting</html></body></h1>";
---
> 		_body = "<html><body><h1>" + itos(_ncode) + " " + getReason(_ncode) + "</h1></body></html>";
27c28,32
< 	if (!file.is_open())
---
> 	// page d'erreur configurée mais introuvable → retomber sur la page
> 	// par défaut plutôt que de renvoyer un body vide
> 	if (!file.is_open()) {
> 		_type = getType(".html");
> 		_body = "<html><body><h1>" + itos(_ncode) + " " + getReason(_ncode) + "</h1></body></html>";
28a34
> 	}
39c43
<     if (isError(_ncode) && _location.empty() && _body.empty()) {
---
>     if (!_noBody && isError(_ncode) && _location.empty() && _body.empty()) {
45d48
< 	std::cout << "NCODE" << _ncode << std::endl;
56c59
< 		<< "Connection: close\r\n"
---
> 		<< "Connection: " << (_keepAlive ? "keep-alive" : "close") << "\r\n"
```

### `srcs/http/HttpParser.cpp`

```diff
3,9c3
< 
< 
< HttpParser::HttpParser(size_t maxBodyClient) : _errorCode(0), _state(R_HEADERS),
< 											   _bodyExcepted(0), _bodyReceived(0),
< 											   _maxBodySize(maxBodyClient) {}
< 
< 
---
> HttpParser::HttpParser(const Server& server) : _errorCode(0), _state(R_HEADERS), _bodyExcepted(0), _bodyReceived(0), _maxBodySize(server.max_body_client), _server(&server) {}
17c9,11
< 	_req.error = 200;
---
> 	// ne pas écraser une erreur déjà flaggée (ex: 413 détecté sur un chunk,
> 	// alors que le terminateur "0\r\n\r\n" arrive dans un recv() ultérieur)
> 	_req.error = (_errorCode != 0) ? _errorCode : 200;
23c17,21
<         if (pos == std::string::npos) 
---
>         if (pos == std::string::npos) {
> 			// garde-fou : des headers sans fin ne doivent pas faire grossir
> 			// le buffer indéfiniment (résilience exigée par le sujet)
> 			if (_buffer.size() > MAX_HEADER_SIZE)
> 				setError(431);
24a23
> 		}
36,46c35,53
< 		_body += _buffer;
< 		_buffer.clear();
< 		_bodyReceived = _body.size();
< 		if (_bodyReceived > _bodyExcepted)
< 			setError(400);
< 		else if (_bodyReceived == _bodyExcepted) {
< 			_req.error = 200;
< 			_req.body = _body;
< 			if (_req.isMultipart == true)
< 				getMp();
< 			_state = COMPLETE;
---
> 		size_t need = _bodyExcepted - _bodyReceived;
> 		size_t take = (_buffer.size() < need) ? _buffer.size() : need;
> 		if (_errorCode == 413) {
> 			// corps trop gros : jeter les données au lieu de les
> 			// accumuler, pour ne pas exploser la mémoire
> 			_buffer.erase(0, take);
> 		} else {
> 			_body.append(_buffer, 0, take);
> 			_buffer.erase(0, take);
> 		}
> 		_bodyReceived += take;
> 		if (_bodyReceived == _bodyExcepted) {
> 			_req.error = (_errorCode != 0) ? _errorCode : 200;
> 			if (_errorCode == 0) {
> 				_req.body = _body;
> 				if (_req.isMultipart == true)
> 					getMp();
> 			}
> 			_state = (_errorCode == 0) ? COMPLETE : ERROR;
58c63
< 	_req.error = errorCode; 
---
> 	_req.error = errorCode;
60c65,68
< 	if (_state != R_CHUNKED)
---
> 	// Pour 413 en mode normal, ne pas changer l'état immédiatement :
> 	// on continue de lire le corps jusqu'à la fin (en le jetant) pour
> 	// que le client puisse terminer son envoi avant de lire notre 413.
> 	if (_state != R_CHUNKED && errorCode != 413)
110a113,118
> 	// la limite de body dépend de la location matchée (client_max_body_size
> 	// par location) : on la résout dès les headers, avant de lire le body
> 	const Location* loc = findLocation(_req.uri.substr(0, _req.uri.find('?')), *_server);
> 	if (loc)
> 		_maxBodySize = loc->max_body_client;
> 
119c127,129
< 		if (it->second == "chunked")
---
> 		// tester la CLÉ Transfer-Encoding : avant, n'importe quel header dont
> 		// la valeur était "chunked" (ex: X-Foo: chunked) déclenchait le mode
> 		if (it->first == "Transfer-Encoding" && it->second.find("chunked") != std::string::npos)
158a165,166
> 		// end++ : avancer le POINTEUR. L'ancien (*end)++ incrémentait le
> 		// caractère pointé (UB : écriture dans le buffer de c_str()).
160c168
< 			(*end)++;
---
> 			end++;
162c170,171
< 		if (*end != '\0')
---
> 		// ';' introduit une chunk-extension (RFC 7230), autorisée → ignorée
> 		if (*end != '\0' && *end != ';')
165,168d173
< 		_bodyReceived += size;
< 		if (_bodyReceived > _maxBodySize)
< 			setError(413);
< 
169a175,183
> 			// attendre le terminateur complet "0\r\n\r\n" puis le consommer,
> 			// sinon il traînerait dans _buffer et corromprait la requête
> 			// keep-alive suivante (sauf si on est déjà en erreur : inutile
> 			// d'attendre des octets qui n'arriveront peut-être jamais)
> 			if (_errorCode != 413 && _errorCode != 400) {
> 				if (_buffer.size() < pos + 4)
> 					return;
> 				_buffer.erase(0, pos + 4);
> 			}
182a197,204
> 		// compter le chunk UNIQUEMENT quand il est entier dans le buffer :
> 		// avant, l'incrément se faisait avant le check de complétude, donc un
> 		// chunk arrivant en plusieurs recv() était re-compté à chaque appel
> 		// (jusqu'à ~16x avec des chunks de 64k et un buffer de 4k) → faux 413
> 		_bodyReceived += size;
> 		if (_bodyReceived > _maxBodySize)
> 			setError(413);
> 
207a224,226
> 	// la limite a pu être écrasée par celle d'une location : revenir au
> 	// défaut du serveur pour la prochaine requête keep-alive
> 	_maxBodySize = _server->max_body_client;
```

### `srcs/http/MultipartHandler.cpp`

```diff
12c10,19
<         std::string path = _path + "/" + i->second.filename;
---
>         // ne garder que le nom de fichier : un filename client contenant
>         // "../../x" permettait d'écrire hors du dossier d'upload
>         std::string fname = i->second.filename;
>         size_t slash = fname.rfind('/');
>         if (slash != std::string::npos)
>             fname = fname.substr(slash + 1);
>         if (fname.empty())
>             continue ;
> 
>         std::string path = _path + "/" + fname;
13a21,26
>         std::ofstream	file(path.c_str(), std::ios::binary);
>         if (!file.is_open()) { // dossier absent ou non inscriptible
>             _ncode = 500;
>             body += "<li>" + fname + " failed</li>";
>             continue ;
>         }
16d28
<         std::ofstream	file(path.c_str(), std::ios::binary);
19c31
<         body += "<li>" + path + " uploded</li>";
---
>         body += "<li>" + path + " uploaded</li>";
36c46
< }
---
> }
\ No newline at end of file
```

### `srcs/http/MultipartParser.cpp`

**Aucune différence** (hors lignes vides)

### `srcs/http/Router.cpp`

```diff
16c14,18
<     const Location *loc = bestRouteFound(req.uri, server);
---
>     // matcher la location sur le chemin SANS la query string : avec elle,
>     // "/upload?x=1" ne matchait jamais "/upload" et retombait sur "/"
>     std::string uriPath = req.uri.substr(0, req.uri.find('?'));
> 
>     const Location *loc = bestRouteFound(uriPath, server);
20c22,31
<     std::string uriPath = req.uri.substr(0, req.uri.find('?'));
---
>     if (!methodImplemented(req.method))
>         return new ErrorHandler(*loc, 501);
> 
>     if (!methodAllowed(req.method, loc))
>         return new ErrorHandler(*loc, 405);
> 
>     // la redirection s'applique avant tout accès au filesystem : sinon un
>     // "return" sur une URL sans fichier réel renvoyait 404 au lieu de rediriger
>     if (loc->return_path.first != -1)
>         return new StaticHandler(req, *loc, "");
23c34
<     if (path.empty() && loc->autoindex == 0)
---
>     if (path.empty()) // le chemin normalisé sort de root (ex: /../..)
26,27c37,48
<     if (!methodImplemented(req.method))
<         return new ErrorHandler(*loc, 501);
---
>     // upload : si un dossier de stockage est configuré, c'est lui qui reçoit
>     // les fichiers (exigence du sujet), pas root + URI
>     if (req.method == "POST" && !loc->upload_store.empty()) {
>         if (req.isMultipart == true)
>             path = loc->upload_store;
>         else {
>             size_t slash = uriPath.rfind('/');
>             std::string base = (slash == std::string::npos) ? uriPath : uriPath.substr(slash + 1);
>             if (!base.empty())
>                 path = loc->upload_store + "/" + base;
>         }
>     }
32,34d52
<     if (!methodAllowed(req.method, loc))
<         return new ErrorHandler(*loc, 405);
< 
38,42c56,57
<     if (!methodAllowed(req.method, loc))
< 		return new ErrorHandler(*loc, 405);
< 
< 	std::string interpreter = isCgi(uriPath, loc);
< 	if (!interpreter.empty()) {
---
>     std::string interpreter = isCgi(uriPath, loc);
>     if (!interpreter.empty() && req.method == "POST") {
44,45c59,60
< 		return NULL;
< 	}
---
>         return NULL;
>     }
67,68c80,83
< 
< 
---
> // normalizePath() supprime le slash final : tester path[len-1] == '/' ne
> // détectait donc JAMAIS un dossier, et l'index n'était jamais ajouté
> // (GET / renvoyait une redirection vers le chemin filesystem au lieu de
> // servir l'index). On teste réellement le dossier avec isDir().
73,78c88,92
<     std::string newpath = path;
<     if (path[path.length() - 1] == '/') {
<         if (!loc->index.empty())
<             newpath = newpath + loc->index;
<         else if (!isDir(newpath))
<             return "";
---
>     if (isDir(path) && !loc->index.empty()) {
>         std::string withIndex = path + "/" + loc->index;
>         // index inexistant + autoindex on → servir le listing du dossier
>         if (fileFound(withIndex) || loc->autoindex != 1)
>             return withIndex;
80c94
<     return newpath;
---
>     return path;
112,137c120
<     std::string shorturi = uri;
<     const Location *loc = NULL; 
<     int len = -1;
<     if (uri[0] != '/')
<         return NULL;
<     while (true) {
<         for (size_t i = 0; i < server.locations.size(); i++) {
<             if (server.locations[i].path.find(shorturi) == 0 && 
<                 server.locations[i].path == shorturi && 
<                 len < (int)(server.locations[i].path.length())) {
<                 loc = &server.locations[i];
<                 len = (int)(server.locations[i].path.length());
<             } 
<         }
<         if (len != -1)
<             break;
< 
<         if (shorturi.empty())
<             break;
< 
<         size_t i = shorturi.rfind('/');
<         shorturi.erase(i);
<         if (shorturi.empty())
<             shorturi = "/";
<     }
<     return loc;
---
>     return findLocation(uri, server);
143c124
<     return (method == "GET" || method == "DELETE" || method == "POST");
---
>     return (method == "GET" || method == "DELETE" || method == "POST" || method == "HEAD");
```

### `srcs/http/StaticHandler.cpp`

```diff
14,17d11
< 	std::map<int, std::string>::const_iterator it = loc.error_page.find(_ncode);
<     if (it != loc.error_page.end()) {
<      	_errorpage = it->second;
< 	}
23,24c17,21
< 		if (path[path.length() - 1] != '/') {
< 			const std::pair<int, std::string> toreturn(301,path + '/');
---
> 		// rediriger sur l'URI, pas sur le chemin filesystem : avant, la
> 		// Location renvoyée était "/var/www/html/" → 404 chez le client
> 		std::string uriPath = req.uri.substr(0, req.uri.find('?'));
> 		if (uriPath[uriPath.length() - 1] != '/') {
> 			const std::pair<int, std::string> toreturn(301, uriPath + '/');
26c23
< 		} else
---
> 		} else if (loc.autoindex == 1) // autoindex n'était jamais vérifié ici
28c25,26
< 		return ;
---
> 		else
> 			_ncode = 403;
30c28
< 	if (req.method == "GET")
---
> 	else if (req.method == "GET")
36,37d33
< }
< 
38a35,41
> 	// chercher la page d'erreur configurée pour le code FINAL : avant, la
> 	// recherche se faisait en début de constructeur avec _ncode encore à 200,
> 	// donc les error_page n'étaient jamais utilisées par ce handler
> 	std::map<int, std::string>::const_iterator it = loc.error_page.find(_ncode);
>     if (it != loc.error_page.end())
>      	_errorpage = it->second;
> }
40c43,44
< StaticHandler::StaticHandler(const Location& loc, int code, const std::string& body) {
---
> StaticHandler::StaticHandler(const Location& loc, int code, const std::string& body)
> 	: _req(NULL), _loc(&loc) { // pointeurs laissés non initialisés avant (UB latent)
57c59,63
< 	if (!file.is_open())
---
> 	// le fichier existe (vérifié par le Router) mais ne s'ouvre pas :
> 	// droits insuffisants → 403, pas un faux 200 avec body vide
> 	if (!file.is_open()) {
> 		_ncode = 403;
> 		_type = "";
58a65
> 	}
68a74,76
> 	// tester l'existence AVANT d'ouvrir : l'ofstream crée le fichier,
> 	// donc l'ancien ordre rendait exist toujours vrai → jamais de 201
> 	bool			exist = fileFound(_path);
71c79,83
< 	bool			exist = fileFound(_path);
---
> 
> 	if (!file.is_open()) { // dossier cible absent ou non inscriptible
> 		_ncode = 403;
> 		return ;
> 	}
78c90
< 	
---
> 
99c107,109
< 	std::string path = _loc->root + _req->uri;
---
> 	// _path est déjà le chemin résolu et normalisé du dossier : le recalculer
> 	// avec root + uri ré-introduisait la query string et cassait l'opendir
> 	std::string path = _path;
105c115,116
< 	if (!dir)
---
> 	if (!dir) {
> 		_ncode = 403;
106a118
> 	}
114c126
< 			html += "<li><a class=\"directory\" href=\"" + listing + "/\">" + listing + "</a><br>\n";
---
> 			html += "<li><a class=\"directory\" href=\"" + listing + "/\">" + listing + "</a></li>\n";
116c128
< 			html += "<li><a class=\"file\" href=\"" + listing + "\">" + listing + "</a><br>\n";
---
> 			html += "<li><a class=\"file\" href=\"" + listing + "\">" + listing + "</a></li>\n";
120c132
< 		_body += "<html><body>Empty directory";
---
> 		_body += "<li>Empty directory</li>\n";
122,123c134,137
< 		_body += html + "</ul>\n";
< 	html += "</body>\n</html>\n";
---
> 		_body += html;
> 	// les balises fermantes étaient ajoutées à la variable locale `html`
> 	// APRÈS sa copie dans _body : elles n'atteignaient jamais la réponse
> 	_body += "</ul>\n</body>\n</html>\n";
```

### `srcs/utils/utils.cpp`

```diff
20,21c16,18
< 		case 405: return "Method not allowed";
< 		case 413: return "Body size too large";
---
> 		case 405: return "Method Not Allowed";
> 		case 413: return "Payload Too Large";
> 		case 431: return "Request Header Fields Too Large";
23c20
< 		case 501: return "Method not implemented";
---
> 		case 501: return "Not Implemented";
24a22
>         case 504: return "Gateway Timeout";
92,94c82,83
< 
< 
< std::string itos(int n) {
---
> std::string itos(int n)
> {
100c89,117
< 
---
> // longest-prefix match : réduit l'URI segment par segment jusqu'à trouver
> // une location dont le path correspond exactement (même logique que le
> // Router — partagée pour que HttpParser applique la limite de body de la
> // bonne location dès la lecture des headers, avant de lire le body)
> const Location* findLocation(const std::string& uri, const Server& server) {
>     std::string shorturi = uri;
>     const Location *loc = NULL;
>     int len = -1;
>     if (uri.empty() || uri[0] != '/')
>         return NULL;
>     while (true) {
>         for (size_t i = 0; i < server.locations.size(); i++) {
>             if (server.locations[i].path == shorturi &&
>                 len < (int)(server.locations[i].path.length())) {
>                 loc = &server.locations[i];
>                 len = (int)(server.locations[i].path.length());
>             }
>         }
>         if (len != -1)
>             break;
>         if (shorturi.empty())
>             break;
>         size_t i = shorturi.rfind('/');
>         shorturi.erase(i);
>         if (shorturi.empty())
>             shorturi = "/";
>     }
>     return loc;
> }
144d156
<     std::cout << "newu" << newuri << std::endl;
165,167c173,175
< 
< 
< std::string normalizePath(const std::string& p, const std::string& root) {
---
> // Canonise un chemin : résout "." et "..", supprime les slashs redondants,
> // et préserve le caractère relatif ou absolu du chemin d'origine.
> static std::string canonPath(const std::string& p) {
170a179
>     bool                        absolute = (!p.empty() && p[0] == '/');
176c185
<             if (!sgmt.empty())
---
>             if (!sgmt.empty() && sgmt.back() != "..")
178c187
<             else 
---
>             else
185,189c194,200
<     for (size_t i = 0; i < sgmt.size(); i++)
<         result += "/" + sgmt[i];
< 
<     if (result.find(root) != 0)
<         return "";
---
>     for (size_t i = 0; i < sgmt.size(); i++) {
>         if (i > 0 || absolute)
>             result += "/";
>         result += sgmt[i];
>     }
>     if (result.empty() && absolute)
>         result = "/";
192a204,219
> // Compare le chemin demandé à root sous forme canonique. L'ancien
> // `result.find(root) != 0` avait deux problèmes :
> //   - un simple test de sous-chaîne : "/var/www-evil" passait pour "/var/www"
> //   - root devait être déjà canonique ("./www" ou "www" ne matchaient jamais
> //     leur propre contenu → 403 systématique avec un root relatif)
> std::string normalizePath(const std::string& p, const std::string& root) {
>     std::string res   = canonPath(p);
>     std::string nroot = canonPath(root);
> 
>     if (res.compare(0, nroot.size(), nroot) != 0)
>         return "";
>     // frontière de segment : ce qui suit root doit être "/" ou rien
>     if (res.size() > nroot.size() && res[nroot.size()] != '/')
>         return "";
>     return res;
> }
```

---

## Fichiers uniques à webservPerfect (absents dans webserv)

