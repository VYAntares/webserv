#include <string>

// Interface (pattern Observer) : CGIReadHandler ne connait que ce contrat,
// pas ClientHandler directement, ce qui evite un #include circulaire.
// Ex. dans CGIReadHandler::handle_input(), une fois la reponse du CGI prete :
//     _sink->onCgiDone(r);
// _sink est en realite un ClientHandler* stocke comme IResponseSink*.
// Son override (ClientHandler::onCgiDone) fait juste :
//     _response = rawHttpResp;
//     EventLoop::instance()->modify_handler(this, WRITE_EVENT);
class IResponseSink {
	public:
		virtual void onCgiDone(const std::string& rawHttpResp) = 0;
		virtual ~IResponseSink() {}
};
