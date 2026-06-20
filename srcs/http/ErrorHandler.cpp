#include "../../includes/http/ErrorHandler.hpp"

ErrorHandler::ErrorHandler(const Location& loc, int code) : _error(code) {
	if (loc.return_path.first != -1)
		handleReturn(loc.return_path);

	std::map<int, std::string>::const_iterator it = loc.error_page.find(_error);

	if (it != loc.error_page.end())
    	_errorpage = it->second;
}

ErrorHandler::ErrorHandler(const Server& server, int code) : _error(code){
	if (server.return_path.first != -1) {
		handleReturn(server.return_path);
		return ;
	}
	std::map<int, std::string>::const_iterator it = server.error_page.find(_error);

	if (it != server.error_page.end())
		_errorpage = it->second;
}

void ErrorHandler::handleReturn(const std::pair<int, str>& return_path) {
	_error = return_path.first;
	_type = getType(".html");

	if (!return_path.second.empty()) {
		_body = "<html><body><h1>Redirecting to " + return_path.second + "</html></body></h1>";
		_location = return_path.second;
	}
	else
		_body = "<html><body><h1>Redirecting</html></body></h1>";
}

void	ErrorHandler::getErrorPage() {
	_type = getType(_errorpage);

	std::ifstream file(_errorpage.c_str());
	if (!file.is_open())
		return ;

	std::stringstream ss;
	ss << file.rdbuf();
	_body = ss.str();
	file.close();
}

std::string ErrorHandler::buildResponse() {
    std::string reason = getReason(_error);
	if (_errorpage.length() > 0)
		getErrorPage();
	else
		_body = "<html><body><h1>" + itos(_error) + " " + getReason(_error) + "</h1></body></html>";
	
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _error << " " << reason << "\r\n";

	if ((_error >= 300 && _error < 400) && !_location.empty())
		oss << "Location: " << _location << "\r\n";

	oss	<< "Content-Type: " << _type << "\r\n"
		<< "Content-Length: " << _body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< _body;

	return oss.str();
}
