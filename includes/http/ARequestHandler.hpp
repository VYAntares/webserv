#pragma once

#include <string>

class ARequestHandler {
	public:
		
		std::string			buildResponse();
		virtual				~ARequestHandler() {}

		// devrait etre que dans staticHandler.hpp et errorHandler.hpp
		void				getErrorPage();
		void 				handleReturn(const std::pair<int, std::string>& return_path);

		void				setKeepAlive(bool ka) { _keepAlive = ka; }

	protected:
		int					_ncode;
		std::string			_type;
		std::string			_body;
		bool				_keepAlive;

		// devrait etre que dans staticHandler.hpp et errorHandler.hpp
		std::string			_location;
		std::string			_errorpage;
};
