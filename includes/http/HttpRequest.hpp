#pragma once

#include <iostream>
#include <map>

struct HttpRequest {
	int									error;
	std::string							method;
	std::string							uri;
	std::string							version;
	std::string							body;	
	std::map<std::string, std::string>	headers;
}; 
