#pragma once

#include "../config/ConfigStruct.hpp"
#include "ARequestHandler.hpp"
#include "HttpRequest.hpp"

class StaticHandler : public ARequestHandler {
	public:
		StaticHandler(const Location& loc, int code, const std::string& body);
		StaticHandler(const HttpRequest& req, const Location& loc, const std::string& path);
		~StaticHandler() {};
		void		handleGet();
		void		handlePost();
		void		handleDelete();
		void		throwList();
		void		headerListe(const std::string& path);

		bool		isMultipart();

	private:
		const HttpRequest*	_req;
		const Location*		_loc;
		std::string			_path;
};
