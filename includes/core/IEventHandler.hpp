#pragma once

#include <cstddef>

// --- EventType ---
// Les valeurs sont des puissances de 2 écrites en octal (préfixe 0).
// En octal, chaque chiffre vaut 8x le précédent : 01=1, 02=2, 04=4, 010=8, 020=16, 040=32.
// Ce sont exactement les mêmes puissances de 2 qu'en décimal (1, 2, 4, 8, 16, 32),
// écriture octal reprise fidèlement du papier Schmidt pour rester conforme.
// Chaque valeur n'a qu'un seul bit allumé en binaire :
//   ACCEPT_EVENT  = 01  = 000001
//   READ_EVENT    = 02  = 000010
//   WRITE_EVENT   = 04  = 000100
//   TIMEOUT_EVENT = 010 = 001000
//   SIGNAL_EVENT  = 020 = 010000
//   CLOSE_EVENT   = 040 = 100000
// Cela permet de tester la présence d'un type avec & (ET bit à bit),
// et de combiner plusieurs types avec | (OU bit à bit) si besoin.
enum EventType {
	ACCEPT_EVENT	= 01,
	READ_EVENT		= 02,
	WRITE_EVENT		= 04,
	TIMEOUT_EVENT	= 010,
	SIGNAL_EVENT	= 020,
	CLOSE_EVENT		= 040
};

// --- IEventHandler ---
// getFd()        : pure virtuelle — tout handler doit exposer son fd à EventLoop.
// handle_*       : virtuelles avec corps vide par défaut — chaque handler
//                  n'override que ce dont il a besoin, sans switch ni cast.
//
// Pattern Reactor — Schmidt, 1995.
// Contrat entre EventLoop (Initiation Dispatcher) et les handlers (Concrete Event Handlers).
//
// Sans cette interface, EventLoop devrait connaître chaque type de handler :
//   if (type == ACCEPT)   acceptHandler->handle_accept();
//   else if (type == CGI) cgiHandler->handle_pipe();   // ← modifier EventLoop à chaque ajout
//
// Avec cette interface, EventLoop ne connaît qu'un seul type :
//   IEventHandler* h = events[i].data.ptr;
//   h->handle_input();   // le bon handler est appelé automatiquement via polymorphisme
//
// Ajouter un nouveau handler = implémenter IEventHandler + l'enregistrer.
// EventLoop ne change jamais.
class IEventHandler {
	public:
		virtual int getFd()	const = 0;
		virtual int handle_accept()	{ return 0; }	// AcceptHandler.hpp
		virtual int handle_input()	{ return 0; }	// ClientHandler.hpp, CGIReadHandler.hpp
		virtual int handle_output()	{ return 0; }	// ClientHandler.hpp, CGIWriteHandler.hpp
		virtual int handle_close()	{ return 0; }
		virtual	~IEventHandler() {}
};

