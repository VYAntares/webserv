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

class IEventHandler {
	public:
		virtual int getFd() const  = 0;
		virtual void handle_close()   {}
		virtual void handle_read()	  {}	// ClientHandler.hpp, CGIReadHandler.hpp
		virtual void handle_write()	  {}	// ClientHandler.hpp, CGIWriteHandler.hpp
		virtual void handle_accept()  {}	// AcceptHandler.hpp
		virtual		~IEventHandler()  {}
};
