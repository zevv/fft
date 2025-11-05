
#include "stream-reader.hpp"

class StreamReaderFile : public StreamReader {
public:
	StreamReaderFile(SDL_AudioSpec &dst_spec, SDL_AudioSpec &src_spec, int fd);
	~StreamReaderFile();

	void poll() override;
	void open() override;

private:
	SDL_AudioSpec m_src_spec{};
	int m_fd{-1};
	uint8_t m_buf[65536]{};
	size_t m_buf_bytes{0};
};

