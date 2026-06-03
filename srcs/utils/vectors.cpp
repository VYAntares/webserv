#include "../includes/utils/vectors.hpp"

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