#include "../../includes/core/EventLoop.hpp"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

// Drapeau d'arrêt levé par le signal handler.
// volatile : force la relecture depuis la mémoire à chaque tour (pas de cache registre).
// sig_atomic_t : lecture/écriture atomique, sans risque de valeur à moitié écrite par un signal.
static volatile sig_atomic_t g_stop = 0;

// Handler SIGINT/SIGTERM : lève g_stop pour que handle_events() sorte proprement.
// Seules les fonctions async-signal-safe sont autorisées ici — donc pas de malloc/cout.
static void handle_sigint(int) { g_stop = 1; }

// Traduit notre EventType en flags epoll.
// EventType et les flags epoll sont deux systèmes de constantes différents —
// on ne peut pas passer EventType directement à epoll_ctl.
//
// Chaque constante est une puissance de 2, donc un seul bit est allumé
//
// L'opérateur & (ET bit à bit) teste si un bit est présent dans type :
//   type & READ_EVENT → non-zéro (true) si le bit READ est allumé, 0 sinon.
//
// L'opérateur |= allume un bit dans flags sans effacer les autres,
// ce qui permet d'accumuler plusieurs flags pour un seul appel epoll_ctl.
static uint32_t to_epoll_flags(EventType type) {
	uint32_t flags = 0;
	if (type & (ACCEPT_EVENT | READ_EVENT))	flags |= EPOLLIN;	// surveille les données entrantes
	if (type & WRITE_EVENT)					flags |= EPOLLOUT;	// surveille la disponibilité en écriture
	return flags;
}

EventLoop* EventLoop::_instance = NULL;

EventLoop* EventLoop::instance() {
	if (!_instance)
		_instance = new EventLoop();
	return _instance;
}

// Crée l'instance epoll unique qui surveillera tous les fd de la boucle.
EventLoop::EventLoop() {
	_epfd = epoll_create1(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create() failed: " + std::string(strerror(errno)));
}

// Crée une entrée dans la table : associe le handler à son type d'événement.
// C'est ce pointeur (entry) qu epoll nous rendra dans events[i].data.ptr
// lors du prochain epoll_wait — on sait ainsi quel handler appeler.
// Enregistre le fd auprès d epoll avec le masque d'événements calculé.
// EPOLL_CTL_ADD : premier enregistrement de ce fd (utiliser MOD si déjà présent).
void EventLoop::register_handler(IEventHandler* h, EventType type) {
	HandlerEntry* entry	= new HandlerEntry();
	entry->handler		= h;
	entry->type			= type;
	_table.push_back(entry);

	struct epoll_event ev;
	ev.events	= to_epoll_flags(type);
	ev.data.ptr	= entry;
	epoll_ctl(_epfd, EPOLL_CTL_ADD, h->getFd(), &ev);
}

// Met à jour le type d'événement surveillé pour un handler déjà enregistré.
// Cherche l'entrée dans la table, met à jour son type, puis recalcule les flags
// et appelle EPOLL_CTL_MOD pour que epoll surveille le nouveau masque.
void EventLoop::modify_handler(IEventHandler* h, EventType newType) {
	HandlerEntry* entry = NULL;
	for (size_t i = 0; i < _table.size(); i++) {
		if (_table[i]->handler == h) {
			_table[i]->type = newType;
			entry = _table[i];
			break;
		}
	}
	if (!entry)
		return;

	struct epoll_event ev;
	ev.events	= to_epoll_flags(newType);
	ev.data.ptr	= entry;
	epoll_ctl(_epfd, EPOLL_CTL_MOD, h->getFd(), &ev);
}

// Retire le fd d epoll puis supprime l'entrée de la table interne.
// EPOLL_CTL_DEL n'a pas besoin de struct epoll_event (dernier arg NULL).
// Le handler lui-même n est pas delete ici — c'est à l appelant de le faire
// pour éviter un double-free (handle_events() appelle remove_handler puis delete h).
void EventLoop::remove_handler(IEventHandler* h) {
	epoll_ctl(_epfd, EPOLL_CTL_DEL, h->getFd(), NULL);

	for (size_t i = 0; i < _table.size(); i++) {
		if (_table[i]->handler == h) {
			delete _table[i];
			_table.erase(_table.begin() + i);
			break;
		}
	}
}

// epoll_wait retourne :
//   > 0 : n événements prêts → on traite le for ci-dessous
//     0 : timeout (500ms écoulées sans événement) → on reboucle pour re-vérifier g_stop
//    -1 : erreur, typiquement EINTR quand le signal a interrompu l'appel système
// Dans les deux cas non-positifs on saute le for : events[] n'est pas rempli
// et n vaut 0 ou -1, donc itérer dessus serait UB ou traiterait des entrées vides.
void EventLoop::handle_events() {
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);
	signal(SIGQUIT, handle_sigint);

	std::cout << "Server listening.." << std::endl;
	struct epoll_event events[MAX_EVENTS];

	while (!g_stop) {
		int n = epoll_wait(_epfd, events, MAX_EVENTS, 500);

		if (n <= 0)
			continue;

		for (int i = 0; i < n; i++) {
			HandlerEntry* entry	= static_cast<HandlerEntry*>(events[i].data.ptr);
			IEventHandler* h	= entry->handler;

			int ret = 0;
			if (entry->type == ACCEPT_EVENT) 
				ret = h->handle_accept();
			else if (entry->type == READ_EVENT)
				ret = h->handle_input();
			else if (entry->type == WRITE_EVENT)
				ret = h->handle_output();

			if (ret == -1) {
				remove_handler(h);
				delete h;
			}
		}
	}
}

void EventLoop::destroy() { delete _instance; }

EventLoop::~EventLoop() {
	for (size_t i = 0; i < _table.size(); i++) {
		delete _table[i]->handler;
		delete _table[i];
	}
	_instance = NULL;
}

