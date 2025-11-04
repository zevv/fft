
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "stream-reader-file.hpp"

StreamReaderFile::StreamReaderFile(size_t ch_count, int fd)
	: StreamReader("file", ch_count)
	, m_fd(fd)
{
	m_spec_src.freq = 41000;
	m_spec_src.format = SDL_AUDIO_S16LE;
	m_spec_src.channels = m_ch_count;
}


StreamReaderFile::~StreamReaderFile()
{
	if(m_fd != -1) {
		::close(m_fd);
		m_fd = -1;
	}
}


SDL_AudioStream *StreamReaderFile::do_open(SDL_AudioSpec *spec_dst)
{
	fcntl(m_fd, F_SETFL, O_NONBLOCK);

	SDL_AudioStream *sas = SDL_CreateAudioStream(&m_spec_src, spec_dst);
	if(sas == nullptr) {
		fprintf(stderr, "StreamReaderFile SDL_CreateAudioStream failed: %s\n", SDL_GetError());
	}
	return sas;
}


void StreamReaderFile::do_poll(SDL_AudioStream *sas)
{
	if(m_fd == -1) return;

	ssize_t r = read(m_fd, m_buf + m_buf_bytes, sizeof(m_buf) - m_buf_bytes);
	if(r > 0) {
		m_buf_bytes += r;
		size_t frame_count = m_buf_bytes / m_frame_size;
		SDL_PutAudioStreamData(sas, m_buf, frame_count * m_frame_size);
		
		m_buf_bytes = m_buf_bytes - frame_count * m_frame_size;
		if(m_buf_bytes > 0) {
			memmove(m_buf, m_buf + frame_count * m_frame_size, m_buf_bytes);
		}
	} else {
		if(r < 0) {
			fprintf(stderr, "StreamReaderFile read error: %s\n", strerror(errno));
		}
		::close(m_fd);
		m_fd = -1;
	}
}

