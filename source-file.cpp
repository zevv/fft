
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "source.hpp"
#include "sourceregistry.hpp"
#include "misc.hpp"

class SourceFile : public Source {
public:
	SourceFile(Source::Info &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceFile();

	void poll() override;
	void open() override;

private:
	SDL_AudioSpec m_src_spec{};
	int m_fd{-1};
	uint8_t m_buf[65536]{};
	size_t m_buf_bytes{0};
};


SourceFile::SourceFile(Source::Info &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec, args)
	, m_fd(0)
{
	m_src_spec = sdl_audiospec_from_str(args);
	m_dst_spec.channels = m_src_spec.channels;
}


SourceFile::~SourceFile()
{
	if(m_fd != -1) {
		::close(m_fd);
		m_fd = -1;
	}
}


void SourceFile::open()
{
	fcntl(m_fd, F_SETFL, O_NONBLOCK);

	m_sdl_stream = SDL_CreateAudioStream(&m_src_spec, &m_dst_spec);
	if(m_sdl_stream == nullptr) {
		fprintf(stderr, "SourceFile SDL_CreateAudioStream failed: %s\n", SDL_GetError());
	}
}


void SourceFile::poll()
{
	if(m_fd == -1) return;

	size_t frame_size = m_src_spec.channels * SDL_AUDIO_BYTESIZE(m_src_spec.format);

	ssize_t r = read(m_fd, m_buf + m_buf_bytes, sizeof(m_buf) - m_buf_bytes);
	if(r > 0) {
		m_buf_bytes += r;
		size_t frame_count = m_buf_bytes / frame_size;
		bool ok = SDL_PutAudioStreamData(m_sdl_stream, m_buf, frame_count * frame_size);
		
		m_buf_bytes = m_buf_bytes - frame_count * frame_size;
		if(m_buf_bytes > 0) {
			memmove(m_buf, m_buf + frame_count * frame_size, m_buf_bytes);
		}
	} else if(r == 0) {
		::close(m_fd);
		m_fd = -1;
	} else if(errno != EAGAIN && errno != EWOULDBLOCK) {
		fprintf(stderr, "SourceFile read error: %s\n", strerror(errno));
		::close(m_fd);
		m_fd = -1;
	}
}


REGISTER_STREAM_READER(SourceFile,
	.name = "raw",
	.description = "raw audio file reader",
);

