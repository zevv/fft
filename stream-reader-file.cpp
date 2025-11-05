
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "stream-reader-file.hpp"

StreamReaderFile::StreamReaderFile(size_t ch_count, SDL_AudioFormat fmt, Samplerate srate, int fd)
	: StreamReader("file", ch_count)
	, m_fd(fd)
{
	m_spec_src.freq = srate;
	m_spec_src.format = fmt;
	m_spec_src.channels = ch_count;
}


StreamReaderFile::~StreamReaderFile()
{
	if(m_fd != -1) {
		::close(m_fd);
		m_fd = -1;
	}
}


void StreamReaderFile::open()
{
	fcntl(m_fd, F_SETFL, O_NONBLOCK);

	m_sdl_stream = SDL_CreateAudioStream(&m_spec_src, &m_sdl_audio_spec);
	if(m_sdl_stream == nullptr) {
		fprintf(stderr, "StreamReaderFile SDL_CreateAudioStream failed: %s\n", SDL_GetError());
	}
}


void StreamReaderFile::poll()
{
	if(m_fd == -1) return;

	size_t frame_size = m_spec_src.channels * SDL_AUDIO_BYTESIZE(m_spec_src.format);

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
		fprintf(stderr, "StreamReaderFile read error: %s\n", strerror(errno));
		::close(m_fd);
		m_fd = -1;
	}
}

