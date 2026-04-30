#pragma once

class HttpParser {
	public:
		enum State {
			R_HEADERS,
			R_BODY,
			R_CHUNCKED,
			COMPLETE,
			ERROR
		};

		explicit HttpParser(size_t maxBodySize);
		~HttpParser();

		State	getState();
		void	runParsing(std::string& buffer, size_t n);

	private:
		State							_state;
		std::string						_buffer;

		HttpRequest						_req;

		size_t							_bodyExcepted;
		size_t							_bodyReceived;
		size_t							_maxBodySize;
};
