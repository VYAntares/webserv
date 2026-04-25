Vue d'ensemble
Structure de dossiers — saine et bien découpée : core/ (EventLoop + interface), handlers/ (concrete handlers), config/, http/, cgi/, utils/. Le découpage colle au pattern Reactor.

Mapping Reactor → ton code (conforme au papier Schmidt) :

Pattern	Ton code
Initiation Dispatcher	EventLoop (Singleton, conforme §9.5)
Synchronous Event Demultiplexer	epoll_wait
Event Handler (interface)	IEventHandler — multi-method (§9.4)
Concrete Acceptor	ServerHandler
Concrete Handler	ClientHandler, CGIRead/Write, Timer
L'auto-enregistrement dans le constructeur (ServerHandler::ServerHandler → register_handler) reproduit fidèlement l'exemple Logging_Acceptor du papier (lignes 273-283). 👍

Points solides
Multi-method interface (handle_accept/input/output/close) avec defaults vides — bon choix, évite les switch (§9.4 du papier).
EventType en octal matchant le papier.
Singleton EventLoop avec ctor/dtor privés.
RAII dans ServerHandler : try/catch pour fermer le fd si bind/listen échoue.
SO_REUSEADDR + O_NONBLOCK dès la création du socket — correct.
Problèmes majeurs à corriger
1. EventLoop n'enregistre que EPOLLIN (EventLoop.cpp:30)

ev.events = EPOLLIN;  // ❌ peu importe le type passé
Le sujet exige : "poll() doit surveiller lecture ET écriture simultanément". Quand tu voudras écrire la réponse HTTP ou pousser dans le pipe CGI, il faudra EPOLLOUT. Mappe EventType → flags epoll :


uint32_t flags = 0;
if (type & (ACCEPT_EVENT|READ_EVENT)) flags |= EPOLLIN;
if (type & WRITE_EVENT)               flags |= EPOLLOUT;
2. Un fd ne peut pas basculer READ ↔ WRITE
Ta HandlerEntry { handler, type } fige le type à l'enregistrement. Or un ClientHandler doit lire la requête (READ) puis écrire la réponse (WRITE) sur le même fd. Deux options :

Faire un EventLoop::modify_handler(h, newType) qui appelle EPOLL_CTL_MOD.
Plus propre : ne pas stocker type dans la table, et dans handle_events regarder events[i].events & EPOLLIN/EPOLLOUT pour appeler handle_input ou handle_output. C'est le dispatch que fait NGINX.
3. handle_events ne dispatche que ACCEPT et READ (EventLoop.cpp:58-69)
Manquent : WRITE_EVENT, CLOSE_EVENT, EPOLLHUP/EPOLLERR. Le sujet est explicite sur la gestion propre des déconnexions.

4. Ownership fragile
EventLoop::_instance n'est jamais delete (leak en fin de programme — pas dramatique mais à noter).
handle_events fait delete h après remove_handler, et ~EventLoop aussi delete _table[i]->handler. Si un handler quitte la table avant la destruction de l'EventLoop c'est OK, mais le pattern est fragile. Documente clairement : « le handler s'enregistre lui-même → l'EventLoop devient propriétaire ».
5. bindAddress (ServerHandler.cpp:49)
addr.sin_addr.s_addr = listen.first; — fonctionne pour INADDR_ANY (0), mais pour une IP explicite il faudra que la config stocke déjà la valeur en network byte order (ou tu fais htonl ici). Ajoute un commentaire pour figer la convention.

Trous fonctionnels (normal au stade actuel)
ConfigParser.cpp : parse() est commenté. Token.hpp existe mais aucun lexer. Le sujet demande des virtual servers, locations, error_pages, autoindex, redirect, CGI… ton ConfigStruct.hpp couvre déjà tout ça, c'est solide.
CGIReadHandler.hpp, CGIWriteHandler.hpp, TimerHandler.hpp : vides.
Pas de parser HTTP, pas de Request/Response dans http/.
ClientHandler::handle_input fait un seul recv sans buffer cumulatif — squelette à étendre pour les requêtes multi-paquets et le Content-Length/chunked.
Pas de main.cpp visible.
Verdict
Très bonne base : la structure de classes, l'interface IEventHandler multi-méthode, le Singleton EventLoop et le câblage Acceptor → Handler sont conformes au papier Schmidt et au sujet. Les classes ont du sens.

Les deux choses à régler avant d'aller plus loin sont (1) le support de EPOLLOUT/WRITE_EVENT dans EventLoop et (2) la possibilité pour un client de basculer READ↔WRITE sur le même fd via EPOLL_CTL_MOD. Sans ça, tu ne pourras pas écrire de réponse HTTP ni gérer le CGI.