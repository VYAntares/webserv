#pragma once

#include "../core/IEventHandler.hpp"
#include "../config/ConfigStruct.hpp"
#include "../http/HttpParser.hpp"
#include "../http/ARequestHandler.hpp"
#include "../core/IResponseSink.hpp"
#include <string>
#include <netinet/in.h>
#include <ctime>

class ClientHandler : public IEventHandler, public IResponseSink {
	public:
		ClientHandler(int clientFd, const Server& server, const struct sockaddr_in& peerAddr);
		~ClientHandler();

		int		getFd()				const;
		int		handle_input();
		int		handle_output();
		time_t	getLastActivity()	const;
		void	onCgiDone(const std::string& rawHttpResp);
		void	onCgiStart(CGIReadHandler* rd);

	private:
		int					_fd;
		size_t				_sent;
		Server				_server;
		HttpParser			_parser;
		ARequestHandler*	_rh;        // owned — deleted after response is sent
		std::string			_response;  // cached once per request, avoids re-building on each send
		std::string			_peerAddr;  // "ip:port" — used for logs and CGI REMOTE_ADDR

		bool				_keepAlive;   // false → close after send
		time_t				_lastActivity;
		CGIReadHandler*		_cgiRead;

		void				_reset();   // resets per-request state for keep-alive
		std::string			_buildPeerStr(const struct sockaddr_in& addr) const;
		void				_handleComplete();
		void				_handleError();
};
