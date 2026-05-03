#pragma once

// HTTPParser   →   HTTPRequest   →   Router   →   IRequestHandler
// (stateful,       (pure data,       (choisit       (Static, CGI,
//  accumule,        méthode,          le bon          Error...)
//  parse)           uri, headers,     handler)
//                   body)

#include "HttpRequest.hpp"
#include <iostream>

class HttpParser {
	public:
		enum State {
			R_HEADERS,
			R_BODY,
			R_CHUNCKED,
			COMPLETE,
			ERROR
		};

		explicit HttpParser(size_t maxBodySize);
		~HttpParser();

		enum State	getState();
		HttpRequest	getReq();
		void		setError(int errorCode);
		void		checkFirstLine();
		void		runParsing(std::string& buffer, size_t n);
		void		headerParser();

	private:
		int								_errorCode;
		State							_state;
		std::string						_buffer;
		std::string						_body;
		std::string						_header;
	
		HttpRequest						_req;

		size_t							_bodyExcepted;
		size_t							_bodyReceived;
		size_t							_maxBodySize;
};
