#pragma once

#include "HttpRequest.hpp"
#include "../../includes/http/MultipartParser.hpp"
#include "../../includes/utils/utils.hpp"
#include "../config/ConfigStruct.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

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

		void			setBoundary(const std::string& boundary);
		void			runParsing(std::string& buffer);
		void			endOfChunked(size_t pos);
		void			setError(int errorCode);
		void			checkFirstLine();
		void			headerParser();
		void			findHeaders();
		void			readChunked();
		void			treatHeader();
		void			treatBody();
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

