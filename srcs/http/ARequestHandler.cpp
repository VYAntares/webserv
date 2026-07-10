#include "../../includes/http/ARequestHandler.hpp"
#include "../../includes/utils/utils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

void ARequestHandler::handleReturn(const std::pair<int, std::string>& return_path) {
	_ncode = return_path.first;
	_type = getType(".html");

	if (!return_path.second.empty()) {
		_body = "<html><body><h1>Redirecting to " + return_path.second + "</html></body></h1>";
		_location = return_path.second;
	}
	else
		_body = "<html><body><h1>Redirecting</html></body></h1>";
}



void	ARequestHandler::getErrorPage() {
	_type = getType(_errorpage);

	std::ifstream file(_errorpage.c_str());
	if (!file.is_open())
		return ;

	std::stringstream ss;
	ss << file.rdbuf();
	_body = ss.str();
	file.close();
}



std::string ARequestHandler::buildResponse() {
    if (isError(_ncode) && _location.empty() && _body.empty()) {
        if (_errorpage.length() > 0)
            getErrorPage();
        else
            _body = "<html><body><h1>" + itos(_ncode) + " " + getReason(_ncode) + "</h1></body></html>";
    }
	std::cout << "NCODE" << _ncode << std::endl;
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _ncode << " " << getReason(_ncode) << "\r\n";

	if (!_location.empty())
		oss << "Location: " << _location << "\r\n";

	if (!_type.empty())
		oss	<< "Content-Type: " << _type << "\r\n";

	oss << "Content-Length: " << _body.size() << "\r\n"
		<< "Connection: " << ((_keepAlive) ? "keep-alive" : "close") << "\r\n"
		<< "\r\n"
		<< _body;

	return oss.str();
}

