#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/cgi/CGIProcess.hpp"
#include "../../includes/core/EventLoop.hpp"
#include "../../includes/handlers/CGIWriteHandler.hpp"
#include "../../includes/handlers/CGIReadHandler.hpp"

// ================ CGIHandler
//
//
// CGIHandler est l'orchestrateur qui va lancer le processus enfant correctement
// et brancher les pipes dans EventLoop
//
// Le script CGI est un programme à part (Python, PHP...) qui ne comprend
// rien à HTTP : il attend juste qu'on lui donne du texte en entrée, et il
// répond en affichant du texte en sortie, comme n'importe quel programme
// lancé dans un terminal. "Le mettre en stdin" veut dire : ce qu'on écrit
// dans le pipe arrive à l'enfant exactement comme s'il l'avait lu au
// clavier — concrètement, c'est le body de la requête du client (par
// exemple les champs d'un formulaire) qu'on lui transmet ainsi.
// Ce que l'enfant écrit de son côté (un print, un echo...) part dans sa
// sortie standard, qu'on récupère au fur et à mesure : une fois le
// script terminé, ce texte devient le corps de la réponse HTTP renvoyée
// au client.
//
// EventLoop nous prévient juste "il se passe quelque chose sur ce fd",
// sans dire quoi ni combien — à nous de lire/écrire pour savoir :
//   - CGIWriteHandler est prévenu quand on peut écrire dans le stdin de
//     l'enfant sans bloquer tout le serveur ; il envoie alors un bout du
//     body, et sera re-prévenu plus tard pour la suite si tout n'est pas
//     parti d'un coup.
//   - CGIReadHandler est prévenu quand il y a quelque chose à lire dans
//     le stdout de l'enfant. Soit c'est un vrai morceau de sa réponse,
//     soit c'est le signe que l'enfant a fini et fermé sa sortie (plus
//     rien à lire) — et c'est ce deuxième cas qui nous dit "le script a
//     terminé, on peut construire la réponse".
CGIHandler::CGIHandler(const HttpRequest& req, std::string& path, std::string& interpreter,
						const std::string& peerAddr, const Location* loc, IResponseSink* sink) {
	// Processus CGI (fork + exec + pipes)
	_process = new CGIProcess(req, path, interpreter, peerAddr);
	
	// brancher les pipes
	EventLoop::instance()->register_handler(
			new CGIWriteHandler(_process->getWriteFd(), req.body), WRITE_EVENT);
	CGIReadHandler* rd = new CGIReadHandler(_process->getReadFd(), _process->getPid(), loc, sink);
	EventLoop::instance()->register_handler(rd, READ_EVENT);
	// le client doit connaître son CGIReadHandler pour pouvoir se détacher
	// si l'un des deux meurt avant l'autre (voir IResponseSink)
	if (sink)
		sink->onCgiStart(rd);
}



CGIHandler::~CGIHandler() {
	delete _process;
}

