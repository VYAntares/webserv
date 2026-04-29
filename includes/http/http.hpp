#pragma once

#include <iostream>
#include <map>

enum class Method {
	GET,
	POST,
	DELETE,
};

enum class State {
	R_HEADERS,
	R_BODY,
	R_CHUNCKED,
	COMPLETE,
	ERROR
};

struct HttpRequest {
	Method								method;
	std::string							uri;
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
		size_t						_bodyRead;
		size_t						_bodyReceived;
		size_t						maxbodysize;
};


