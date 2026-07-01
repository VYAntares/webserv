#include "../../includes/http/MultipartHandler.hpp"

MultipartHandler::MultipartHandler(const HttpRequest& req, const Location& loc, const std::string& path): _req(&req), _loc(&loc), _path(path) {
    _ncode = 200;
    std::string body;
    std::map<std::string, UploadedFile>::const_iterator i;
    for (i = _req->mp.uploadedFiles.begin(); i != _req->mp.uploadedFiles.end(); i++) {
        std::string path = _path + "/" + i->second.filename;
        bool			exist = fileFound(path);
        if (!exist)
            _ncode = 201;
        std::ofstream	file(path.c_str(), std::ios::binary);
        file.write(i->second.data.c_str(), i->second.size);
        file.close();
        body += "<li>" + path + " uploded</li>";
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