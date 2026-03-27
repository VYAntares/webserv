#pragma once

#include "ASocket.hpp"

class ClientSocket : public ASocket {
	private:
		ClientSocket(const ClientSocket& src);
		ClientSocket& operator=(const ClientSocket& rhs);

	public:
		explicit ClientSocket(int fd);
		~ClientSocket();
		
		void handleRead();
		void handleWrite();
		void handleError();

		bool wantsToRead() const;
		bool wantsToWrite() const;
		bool isTimedOut(time_t currentTime) const;
		bool isFinished() const;
};