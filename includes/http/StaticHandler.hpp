#pragma once

#include "../config/ConfigStruct.hpp"
#include "ARequestHandler.hpp"
#include "HttpRequest.hpp"
#include <unistd.h> 

class StaticHandler : public ARequestHandler {
	public:
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
