#pragma once

#include <ctime>

class ASocket {
	private:
		ASocket(const ASocket& src);
		ASocket& operator=(const ASocket& rhs);
		
	protected:
		int		_fd;
		time_t	_lastActivity;
		bool	_finished;

		void	updateActivity();
		void	markFinished();
	
	public:
		explicit ASocket(int fd);
		virtual  ~ASocket();

		int getFd() const;

		virtual void handleRead() = 0;
		virtual void handleWrite() = 0;
		virtual void handleError() = 0;

		virtual bool wantsToRead() const = 0;
		virtual bool wantsToWrite() const = 0;
		virtual bool isTimedOut(time_t currentTime) const = 0;
		virtual bool isFinished() const = 0;
};
