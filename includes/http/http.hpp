#pragma once

#include <iostream>
#include <map>

enum class Method {
	GET,
	POST,
	DELETE,
};

enum class State {
	reading_headers,
	readig_chunked,
	reading_body,
	complete,
	error,
};

struct HttpRequest {
	Method								method;
	std::string							target;
	std::string							body;	
	std::map<std::string, std::string>	headers;
}; 

class ParseHttp {
	public:
		ParseHttp();
		~ParseHttp();
		Method	getState();

	private:
		State							_state;
		std::string						_buffer;
		HttpRequest						_req;
		std::size_t						body_read;
		std::size_t						body_received;
		std::size_t						maxbodysize;
};


