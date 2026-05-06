#include "http/ErrorHandler.hpp"

ErrorHandler::ErrorHandler(int code) {
	(void)code;
}

ErrorHandler::~ErrorHandler() {}


std::string ErrorHandler::buildResponse() {
    return "";
}