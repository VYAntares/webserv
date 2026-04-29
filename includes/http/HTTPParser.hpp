#pragma once

#include <iostream>
#include <map>

class HTTPParser {
	public:
		enum class State {
			R_HEADERS,
			R_BODY,
			R_CHUNCKED,
			COMPLETE,
			ERROR
		};

		explicit HTTPParser(size_t maxBodySize);
		~HTTPParser();
		Method	getState();

	private:
		State							_state;
		std::string						_buffer;
		HttpRequest						_req;

		size_t							_bodyExcepted;
		size_t							_bodyReceived;
		
		size_t							_maxBodySize;
};
