#pragma once

#include <string>

class CGIReadHandler;

// Interface (pattern Observer) : CGIReadHandler ne connait que ce contrat,
// pas ClientHandler directement, ce qui evite un #include circulaire.
// Ex. dans CGIReadHandler::handle_input(), une fois la reponse du CGI prete :
//     _sink->onCgiDone(r);
// _sink est en realite un ClientHandler* stocke comme IResponseSink*.
// Son override (ClientHandler::onCgiDone) fait juste :
//     _response = rawHttpResp;
//     EventLoop::instance()->modify_handler(this, WRITE_EVENT);
//
// onCgiStart() permet au client de connaitre le CGIReadHandler qui lui
// repondra : indispensable pour se "detacher" l'un de l'autre si l'un des
// deux est detruit en premier (deconnexion client ou timeout CGI), sinon
// l'autre garderait un pointeur pendouillant (use-after-free).
class IResponseSink {
	public:
		virtual void onCgiDone(const std::string& rawHttpResp) = 0;
		virtual void onCgiStart(CGIReadHandler* rd) { (void)rd; }
		virtual ~IResponseSink() {}
};
