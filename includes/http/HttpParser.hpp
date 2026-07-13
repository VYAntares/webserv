#pragma once

#include "HttpRequest.hpp"
#include "../../includes/http/MultipartParser.hpp"
#include "../../includes/utils/utils.hpp"
#include "../config/ConfigStruct.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

// HTTPParser   →   HTTPRequest   →   Router   →   ARequestHandler
// (stateful,       (pure data,       (choisit       (Static, CGI,
//  accumule,        méthode,          le bon          Error...)
//  parse)           uri, headers,     handler)
//                   body)

// taille max de la section headers d'une requête (protection mémoire)
#define MAX_HEADER_SIZE 32768

class HttpParser {
	public:
		enum State {
			R_HEADERS,
			R_BODY,
			R_CHUNKED,
			COMPLETE,
			ERROR
		};

		explicit 		HttpParser(const Server& server);
		~HttpParser();

		enum State		getState();
		HttpRequest		getReq();

		void			runParsing(std::string& buffer);
		void			treatHeader();
		void			treatBody();
		void			setBoundary(const std::string& boundary);
		void			setError(int errorCode);
		void			checkFirstLine();
		void			headerParser();
		void			readChunked();
		void 			getMp();
		void			reset();

	private:
		int				_errorCode;
		State			_state;
		std::string		_buffer;
		std::string		_body;
		std::string		_header;
	
		HttpRequest		_req;


		size_t			_bodyExcepted;
		size_t			_bodyReceived;
		size_t			_maxBodySize;

		const Server*	_server;
};

