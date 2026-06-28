#include "../includes/utils/utils.hpp"

static std::map<std::string, std::string> mime_types = init_mime_types();

std::string getReason(int code) {
    
    switch (code) {
		case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
		case 405: return "Method not allowed";
		case 413: return "Body size too large";
        case 500: return "Internal Server Error";
		case 501: return "Method not implemented";
        default:  return "Unknown";
	}
}

std::string getType(const std::string& path) {
	std::string	ext;

	size_t pos = path.rfind(".");
	if (pos == std::string::npos)
		return "application/octet-stream";

	ext = path.substr(pos);
	
	if (mime_types.count(ext))
		return mime_types[ext];

	return "application/octet-stream";
}

bool isDir(const std::string& path) {
	struct stat st;

	if (stat(path.c_str(), &st) == -1)
		return false;
	return S_ISDIR(st.st_mode);
}

std::map<std::string, std::string> init_mime_types() {
    std::map<std::string, std::string> ext;

    // Texte
    ext[".html"] = "text/html";
    ext[".css"]  = "text/css";
    ext[".js"]   = "application/javascript";
    ext[".json"] = "application/json";
    ext[".txt"]  = "text/plain";
    ext[".xml"]  = "application/xml";

    // Images
    ext[".png"]  = "image/png";
    ext[".jpg"]  = "image/jpeg";
    ext[".jpeg"] = "image/jpeg";
    ext[".gif"]  = "image/gif";
    ext[".svg"]  = "image/svg+xml";
    ext[".ico"]  = "image/x-icon";

    // Application
    ext[".pdf"]  = "application/pdf";
    return ext;
}

bool	fileFound(const std::string& path) {
	struct stat forbuf;

	if (stat(path.c_str(), &forbuf) == -1)
		return false;
	return true;
}

std::string itos(int n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::string getParentDirectory(const std::string& path) {
    size_t pos = path.rfind('/');
    if (pos == std::string::npos)
        return path;
    if (pos == 0)
        return "/";
    return path.substr(0, pos);
}

static int hexaToStr(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

std::string decodeHexa(const std::string& uri, bool plus) {
    std::string newuri;

    for (size_t i = 0; i < uri.length(); i++) {

        if (plus == 1 && uri[i] == '+')
            newuri += ' ';
        else if (uri[i] == '%' && i + 2 < uri.length()) {
            int first = hexaToStr(uri[i + 1]);
            int second = hexaToStr(uri[i + 2]);
            if (first != -1 && second != -1) {
                newuri += static_cast<char>(first * 16 + second);
                i += 2;
            } else {
                newuri += uri[i];
            }
        } else
            newuri += uri[i];
    }
    std::cout << "newu" << newuri << std::endl;
    return newuri;
}

bool isEncoded(const std::string& uri) {
    size_t pos = uri.find('%');
    if (pos != std::string::npos)
        return true;
    return false;
}

bool    isError(int code) {
    if (code >= 400 && code < 600)
        return true;
    return false;
}