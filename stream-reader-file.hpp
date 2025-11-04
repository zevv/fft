
#include "stream-reader.hpp"

class StreamReaderFile : public StreamReader {
public:
	StreamReaderFile(size_t ch_count, int fd);
	~StreamReaderFile();

	void poll() override;
	void open() override;

private:
	SDL_AudioSpec m_spec_src{};
	int m_fd{-1};
	uint8_t m_buf[65536]{};
	size_t m_buf_bytes{0};
};

