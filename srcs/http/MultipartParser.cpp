#include "../includes/http/MultipartParser.hpp"

MultipartParser::MultipartParser(std::string& boundary, std::string& body): _boundary(boundary), _body(body) {
    std::cout << "in MPparser" << std::endl;
    std::cout << body << std::endl;
}

Multipart MultipartParser::parsePart() {
    Multipart mp;
    std::vector<Part> prts = parseMultiPart();
    for (std::vector<Part>::iterator it = prts.begin(); it != prts.end(); ++it) {
        if (it->filename.empty())
            mp.uploadedForm[it->name] = it->content;
        else {
            UploadedFile f;
            f.data        = it->content;
            f.contentType = it->contentType;
            f.filename    = it->filename;
            f.size        = it->content.size();
            mp.uploadedFiles[it->name] = f;
        }
    }
    return mp;
}

std::vector<Part> MultipartParser::parseMultiPart() {
	std::vector<Part> parts;

	size_t pos = _body.find("--" + _boundary);

    if (pos == std::string::npos)
        return parts;

    pos += _boundary.length() + 2;
    while (pos < _body.length()) {
        size_t next = _body.find("\r\n--" + _boundary, pos);

        std::string datapart;
        if (next == std::string::npos)
            datapart = _body.substr(pos);
        else
            datapart = _body.substr(pos, next - pos);

        Part p = parsePart(datapart);
        if (!p.name.empty())
            parts.push_back(p);

        if (next == std::string::npos)
            break ;

        pos = next + 4 + _boundary.length();
        if (pos > _body.length())
            break;
    }
    return parts;
}

Part MultipartParser::parsePart(std::string& inside) {
    Part p;
    size_t sep = inside.find("\r\n\r\n");
    if (sep == std::string::npos)
        return p;

    std::string headers = inside.substr(0, sep);
    p.content = inside.substr(sep + 4);

    size_t pos = 0;
    while (pos < headers.length()) {
        size_t end = headers.find("\r\n", pos);
        std::string line;
        if (end == std::string::npos) {
            line = headers.substr(pos);
            pos  = headers.length();
        } else {
            line = headers.substr(pos, end - pos);
            pos  = end + 2;
        }
        if (!line.empty())
            setPart(line, p);
    }
    return p;
}

void MultipartParser::setPart(const std::string& line, Part& p) {
    if (line.find("Content-Disposition:") != std::string::npos) {

        size_t pos = line.find("name=\"");
        if (pos != std::string::npos) {
            pos += 6;
            size_t end = line.find("\"", pos);
            if (end != std::string::npos)
                p.name = line.substr(pos, end - pos);
        }

        size_t pos2 = line.find("filename=\"");
        if (pos2 != std::string::npos) {
            pos2 += 10;
            size_t end = line.find("\"", pos2);
            if (end != std::string::npos)
                p.filename = line.substr(pos2, end - pos2);
        }

    } else if (line.find("Content-Type:") != std::string::npos) {
        size_t pos = line.find("Content-Type:") + 13;
        while (pos < line.length() && line[pos] == ' ')
            pos++;
        p.contentType = line.substr(pos);
    }
}