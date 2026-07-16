#pragma once

#include "../config/ConfigStruct.hpp"
#include "HttpRequest.hpp"
#include <string>

class ARequestHandler {
	public:
		ARequestHandler() : _ncode(200), _keepAlive(false), _noBody(false) {}

		std::string			buildResponse();
		virtual				~ARequestHandler() {}

		// devrait etre que dans staticHandler.hpp et errorHandler.hpp
		void				getErrorPage();
		void				setErrorPage(const BaseBlock& b);
		void 				handleReturn(const std::pair<int, std::string>& return_path);

		void				setKeepAlive(bool ka) { _keepAlive = ka; }
		void				stripBody() { _body.clear(); _noBody = true; }
		void				setType(std::string& type) { _type = type; }
		void				setCookie(std::string& cookie) { _cookie = cookie; }
		
	protected:
		int					_ncode;
		std::string			_type;
		std::string			_body;
		std::string			_cookie;
		bool				_keepAlive;
		bool				_noBody;

		// devrait etre que dans staticHandler.hpp et errorHandler.hpp
		std::string			_location;
		std::string			_errorpage;
};

