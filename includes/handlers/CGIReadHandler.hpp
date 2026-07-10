#pragma once

#include "../core/IEventHandler.hpp"
#include "../handlers/ClientHandler.hpp"

class CGIReadHandler : public IEventHandler {
	public:
		CGIReadHandler(int fd, pid_t pid, const Location* loc, IResponseSink* sink);
		~CGIReadHandler();

		int				getFd()		 const { return _fd; }
		int				handle_input();
		// le CGI participe au timeout de la boucle : un script qui ne répond
		// jamais est tué au bout de CLIENT_TIMEOUT au lieu de pendre à vie
		time_t			getLastActivity() const { return _lastActivity; }

		// appelé par ~ClientHandler si le client meurt avant le CGI :
		// on ne doit plus jamais toucher au sink après ça
		void			detachSink() { _sink = NULL; }

	private:
		void			reapChild(int& status);
		std::string		buildCgiResponse();

		int				_fd;
		pid_t			_pid;
		std::string		_out;
		const Location*	_loc;
		IResponseSink*	_sink; // lie au client originel qui a lancer le cgi
		bool			_reaped;    // waitpid déjà fait (ne pas re-kill un pid recyclé)
		bool			_responded; // une réponse a été envoyée au sink
		time_t			_lastActivity;
};
