#pragma once

#include "../config/ConfigStruct.hpp"
#include "../http/HttpRequest.hpp"
#include <sys/types.h>

class CGIProcess {
	public:
		CGIProcess(const HttpRequest& req, const Location* loc,
					std::string& path, std::string& interpreter,
					const std::string& peerAddr);
		~CGIProcess();

		int 	getWriteFd() const;
		int 	getReadFd()  const;
		pid_t	getPid()	 const;

	private:
		void	CGIFork(int* pipe_stdout, int* pipe_stdin, 
						std::string& interpreter, std::string& path);
		pid_t	_pid;
		int 	_write_fd;
		int 	_read_fd;
};
