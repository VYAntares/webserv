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
		char**	buildEnvp(std::string& path);
		void	addUri(std::vector<std::string>* envp);
		void	addHost(std::vector<std::string>* envp);
		void	addHeaders(std::vector<std::string>* envp);
		char**	convertToCharStarStarBabyyy(std::vector<std::string>* envp);
		void	addRemoteAddr(std::vector<std::string>* envp);

		const HttpRequest&	_req;
		const Location*		_loc;
		const std::string&	_peerAddr;
		pid_t				_pid;
		int 				_write_fd;
		int 				_read_fd;
};
