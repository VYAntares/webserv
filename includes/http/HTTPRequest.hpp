#pragma once

#include <iostream>
#include <map>

struct HttpRequest {
	Method								method;
	std::string							uri;
	std::string							body;	
	std::map<std::string, std::string>	headers;
};
