#include "../../includes/http/MultipartHandler.hpp"
#include "../../includes/utils/utils.hpp"
#include <fstream>

MultipartHandler::MultipartHandler(const Location& loc, const HttpRequest& req, 
									const std::string& path): _req(&req), _path(path) {
    _ncode = 200;
    std::string body;

    std::multimap<std::string, UploadedFile>::const_iterator i;
    for (i = _req->mp.uploadedFiles.begin(); i != _req->mp.uploadedFiles.end(); i++) {
        // ne garder que le nom de fichier : un filename client contenant
        // "../../x" permettait d'écrire hors du dossier d'upload
		std::string fname = i->second.filename;
        size_t slash = fname.rfind('/');
        if (slash != std::string::npos)
			fname = fname.substr(slash + 1);
		if (fname.empty())
			continue ;

        std::string		path  = _path + "/" + fname;
        bool			exist = fileFound(path);
		std::ofstream	file(path.c_str(), std::ios::binary);
        if (!file.is_open()) {
			// dossier absent ou non inscriptible
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

    setErrorPage(loc);
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
