#pragma once

#include "../config/ConfigStruct.hpp"
#include "IEventHandler.hpp"
#include <vector>
#include <sys/epoll.h>

#define MAX_EVENTS 128

// --- HandlerEntry ---
// epoll nous rend un void* (ev.data.ptr) lors d'un événement.
// On y stocke un HandlerEntry* plutôt qu'un IEventHandler* directement,
// pour conserver aussi le type associé et savoir quelle méthode appeler
// (handle_accept, handle_input, handle_output...).
struct HandlerEntry {
	IEventHandler*	handler;
	EventType		type;
};

// EventLoop — Initiation Dispatcher du pattern Reactor (Schmidt, 1995).
//
// Rôle : point central qui surveille tous les fds via epoll, et dispatche
// chaque événement vers le bon handler via polymorphisme (IEventHandler*).
//
class EventLoop {
	public:
		// --- Singleton ---
		// Une seule instance existe dans tout le programme, accessible via instance().
		// Cela permet à n'importe quel handler (ServerHandler, ClientHandler, CGIWriteHandler...)
		// de s'enregistrer ou se retirer sans avoir besoin d'un pointeur vers EventLoop.
		// Ils appellent simplement EventLoop::instance()->register_handler(this, ...).
		static EventLoop* instance();
		static void destroy();

		void register_handler(IEventHandler* h, EventType type);
		void modify_handler(IEventHandler* h, EventType newType);
		void remove_handler(IEventHandler* h);
		void handle_events();

	private:
		EventLoop();
		~EventLoop();

		// --- _epfd ---
		// fd de l'instance epoll unique, créée dans le constructeur via epoll_create1.
		// Tous les fds du serveur (sockets, pipes CGI) sont surveillés via ce seul epfd.
		int							_epfd;

		// --- _table ---
		// Liste de tous les HandlerEntry* enregistrés. EventLoop en est propriétaire :
		// les entries sont détruits dans remove_handler et dans le destructeur.
		// Les handlers eux-mêmes sont détruits par handle_events quand ils retournent -1.
		std::vector<HandlerEntry*>	_table;

		// Implémentation du Singleton :
		//   _instance est un pointeur statique initialisé à NULL.
		//   instance() le crée au premier appel (lazy initialization), puis retourne
		//   toujours le même objet. Le ctor/dtor sont privés : impossible de créer
		//   ou détruire EventLoop depuis l'extérieur.
		static EventLoop* 			_instance;
};

