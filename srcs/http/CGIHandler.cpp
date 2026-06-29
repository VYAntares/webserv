#include "../../includes/http/CGIHandler.hpp"
#include <sstream>

CGIHandler::CGIHandler(const HttpRequest& req, const Location* loc,
						std::string& path, std::string& interpreter,
						const std::string& peerAddr)
						: _req(req), _loc(loc),
						_path(path), _interpreter(interpreter),
						_peerAddr(peerAddr) {}

CGIHandler::~CGIHandler() {}

// std::string CGIHandler::buildResponse() {
// }

