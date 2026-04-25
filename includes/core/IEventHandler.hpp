#pragma once

// Pattern Reactor — Schmidt, 1995.
// Contrat entre EventLoop et handlers.
//
// SANS interface :
//   if (type == ACCEPT)
//       acceptHandler->handle_accept();
//   else if (type == CLIENT)
//       clientHandler->handle_read();
//   else if (type == CGI)      // modif EventLoop
//       cgiHandler->handle_pipe();
//
// AVEC interface :
//   IEventHandler* h = events[i].data.ptr;
//   h->handle_read(); // dispatch automatique
//                     // EventLoop ne change jamais
//
// Ajouter un handler = implementer IEventHandler
// + l'enregistrer. Zero modif de EventLoop.
//
// getFd() = 0 : tout handler a un fd, obligatoire.
// handle_*    : default vide, override si besoin.

#include <cstddef>

enum EventType {
	ACCEPT_EVENT	= 01,
	READ_EVENT		= 02,
	WRITE_EVENT		= 04,
	TIMEOUT_EVENT	= 010,
	SIGNAL_EVENT	= 020,
	CLOSE_EVENT		= 040
};


class IEventHandler {
	public:
		virtual int getFd()	const = 0;
		virtual int handle_accept()	{ return 0; }	// AcceptHandler.hpp
		virtual int handle_input()	{ return 0; }	// ClientHandler.hpp, CGIReadHandler.hpp
		virtual int handle_output()	{ return 0; }	// ClientHandler.hpp, CGIWriteHandler.hpp
		virtual int handle_close()	{ return 0; }
		virtual	~IEventHandler() {}
};
