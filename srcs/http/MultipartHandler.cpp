#include "../../includes/http/MultipartHandler.hpp"
#include "../../includes/utils/utils.hpp"
#include <fstream>

MultipartHandler::MultipartHandler(const HttpRequest& req, const std::string& path): _req(&req), _path(path) {
    _ncode = 200;
    std::string body;
    std::cout << "in handler " << std::endl;
    std::map<std::string, UploadedFile>::const_iterator i;
    for (i = _req->mp.uploadedFiles.begin(); i != _req->mp.uploadedFiles.end(); i++) {
		// ne garder que le nom du fichier.
		std::string fname = i->second.filename;
        size_t slash = fname.rfind('/');
        if (slash != std::string::npos)
			fname = fname.substr(slash + 1);
		if (fname.empty())
			continue ;

        std::string		path  = _path + "/" + fname;
        std::cout << "path > " << path << std::endl;
        bool			exist = fileFound(path);
		std::ofstream	file(path.c_str(), std::ios::binary);
        if (!file.is_open()) {
            _ncode = 500;
            body += "<li>" + fname + " failed</li>";
            continue ;
        }
        if (!exist)
            _ncode = 201;
        file.write(i->second.data.c_str(), i->second.size);
        file.close();
        body += "<li>" + path + " uploaded</li>";
    }
    if (!body.empty())
        setBody(body);
}



void    MultipartHandler::setBody(const std::string& b) {
    _body += "<!DOCTYPE html>\n";
	_body += "<html>\n";
	_body += "<head>\n";
    _body += "<title>Status of uploads</title>\n";
    _body += "</head>\n";
    _body += "<h1>Status of uploads</h1>\n";
    _body += b;
    _body += "</html>\n";
}

