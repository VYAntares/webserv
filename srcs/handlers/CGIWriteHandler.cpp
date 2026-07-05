#include "../../includes/handlers/CGIWriteHandler.hpp"
#include <unistd.h>
#include <iostream>

int CGIWriteHandler::handle_output() {
	if (_sent >= _body.size()) {
		return -1;
	}
	size_t n = write(_fd, _body.data() + _sent, _body.size() - _sent);
	if (n > 0) {
		_sent += static_cast<size_t>(n);
		if (_sent >= _body.size()) {
			return -1;
		}
	}

	// close(_fd);
	return -1;
}