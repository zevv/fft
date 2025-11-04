
#include "stream-reader.hpp"

class StreamReaderFile : public StreamReader {
public:
	StreamReaderFile(size_t ch_count, int fd);
	~StreamReaderFile();

	void do_poll(SDL_AudioStream *sas) override;
	SDL_AudioStream *do_open(SDL_AudioSpec *spec) override;

private:
	SDL_AudioSpec m_spec_src{};
	int m_fd{-1};
	uint8_t m_buf[65536]{};
	size_t m_buf_bytes{0};
};

