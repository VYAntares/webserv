#pragma once

#include <iostream>
#include <map>

struct UploadedFile {
	std::string	data;
	std::string	contentType;
	std::string	filename;
	size_t 		size;
};

struct Multipart {
	std::map<std::string, std::string>	uploadedForm;
	std::map<std::string, UploadedFile>	uploadedFiles;
};

struct HttpRequest {
	int									error;

	std::string							uri;
	std::string							req;
	std::string							body;	
	std::string							method;
	std::string							version;
	std::map<std::string, std::string>	headers;
	

	//Multipart
	Multipart							mp;
	bool								isMultipart;
	std::string							boundary;
}; 
