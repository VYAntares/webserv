#pragma once

#include <string>

class ARequestHandler {
	public:
		ARequestHandler() : _ncode(200), _keepAlive(false), _noBody(false) {}

		std::string			buildResponse();
		virtual				~ARequestHandler() {}

		// devrait etre que dans staticHandler.hpp et errorHandler.hpp
		void				getErrorPage();
		void 				handleReturn(const std::pair<int, std::string>& return_path);

		// keep-alive décidé par ClientHandler ; le header "Connection:" émis
		// par buildResponse doit refléter ce que le serveur fera vraiment.
		void				setKeepAlive(bool ka) { _keepAlive = ka; }
		void				stripBody() { _body.clear(); _noBody = true; }
		// utilisé par CGIReadHandler pour propager le Content-Type du script
		void				setType(const std::string& t) { _type = t; }

	protected:
		int					_ncode;
		std::string			_type;
		std::string			_body;
		bool				_keepAlive;
		bool				_noBody;

		// devrait etre que dans staticHandler.hpp et errorHandler.hpp
		std::string			_location;
		std::string			_errorpage;
};
