#pragma once

#include <sys/socket.h>

class ASocket {
	public:
		virtual				~ASocket() {}
		virtual const int	getFd() const = 0;
		virtual void 		onReadable()  = 0;
		virtual void 		onWritable()  = 0;
};

