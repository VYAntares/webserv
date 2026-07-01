#pragma once

#include <iostream>
#include "HttpRequest.hpp"
#include <vector>

struct Part {
	std::string	content;
	std::string	contentType;
	std::string	filename;
    std::string name;
};

class MultipartParser {
    public:
        MultipartParser(std::string& boundary, std::string& body): _boundary(boundary), _body(body) {}
        ~MultipartParser() {}
        Multipart parsePart();

        void                setPart(const std::string& inside, Part& p);
        Part                parsePart(std::string& inside);
        std::vector<Part>   parseMultiPart();

    private:
        std::string     _boundary;
        std::string     _body;
};
