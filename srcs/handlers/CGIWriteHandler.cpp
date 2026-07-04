#include "../../includes/handlers/CGIWriteHandler.hpp"
#include <unistd.h>
#include <iostream>

int CGIWriteHandler::handle_output() {
	// write la reponse
    
	close(_fd);
	return 0;
}