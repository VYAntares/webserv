#include "../../includes/utils/utils.hpp"

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
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 504: return "Gateway Timeout";
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

// longest-prefix match : réduit l'URI segment par segment jusqu'à trouver
// une location dont le path correspond exactement (même logique que le
// Router — partagée pour que HttpParser applique la limite de body de la
// bonne location dès la lecture des headers, avant de lire le body)
const Location* findLocation(const std::string& uri, const Server& server) {
    std::string shorturi = uri;
    const Location *loc = NULL;
    int len = -1;
    if (uri.empty() || uri[0] != '/')
        return NULL;
    while (true) {
        for (size_t i = 0; i < server.locations.size(); i++) {
            if (server.locations[i].path == shorturi &&
                len < (int)(server.locations[i].path.length())) {
                loc = &server.locations[i];
                len = (int)(server.locations[i].path.length());
            }
        }
        if (len != -1)
            break;
        if (shorturi.empty())
            break;
        size_t i = shorturi.rfind('/');
        shorturi.erase(i);
        if (shorturi.empty())
            shorturi = "/";
    }
    return loc;
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

// Canonise un chemin : résout "." et "..", supprime les slashs redondants,
// et préserve le caractère relatif ou absolu du chemin d'origine.
static std::string canonPath(const std::string& p) {
    std::vector<std::string>    sgmt;
    std::string                 seg;
    std::stringstream           ss(p);
    bool                        absolute = (!p.empty() && p[0] == '/');

    while(std::getline(ss, seg, '/')){
        if (seg == "." || seg.empty())
            continue ;
        else if (seg == "..") {
            if (!sgmt.empty() && sgmt.back() != "..")
                sgmt.pop_back();
            else
                sgmt.push_back(seg);
        }
        else
            sgmt.push_back(seg);
    }
    std::string result;
    for (size_t i = 0; i < sgmt.size(); i++) {
        if (i > 0 || absolute)
            result += "/";
        result += sgmt[i];
    }
    if (result.empty() && absolute)
        result = "/";
    return result;
}

// Compare le chemin demandé à root sous forme canonique. L'ancien
// `result.find(root) != 0` avait deux problèmes :
//   - un simple test de sous-chaîne : "/var/www-evil" passait pour "/var/www"
//   - root devait être déjà canonique ("./www" ou "www" ne matchaient jamais
//     leur propre contenu → 403 systématique avec un root relatif)
std::string normalizePath(const std::string& p, const std::string& root) {
    std::string res   = canonPath(p);
    std::string nroot = canonPath(root);

    if (res.compare(0, nroot.size(), nroot) != 0)
        return "";
    // frontière de segment : ce qui suit root doit être "/" ou rien
    if (res.size() > nroot.size() && res[nroot.size()] != '/')
        return "";
    return res;
}
