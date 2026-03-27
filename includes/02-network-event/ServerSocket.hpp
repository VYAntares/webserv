#pragma once

#include "ASocket.hpp"
#include "EventLoop.hpp"
#include <stdint.h>

class ServerSocket : public ASocket {
	private:
		uint32_t	_host;
		int			_port;
		int			_backlog;
		EventLoop*	_event;

		ServerSocket(const ServerSocket& src);
		ServerSocket& operator=(const ServerSocket& rhs);

		void setupSocket();
		void acceptNewClient();

	public:
		ServerSocket(uint32_t host, int port, EventLoop* event, int backlog = SOMAXCONN);
		~ServerSocket();
		
		void handleRead();
		void handleWrite();
		void handleError();

		bool wantsToRead() const;
		bool wantsToWrite() const;
		bool isTimedOut(time_t currentTime) const;
		bool isFinished() const;
};
