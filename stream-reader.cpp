
#include <string.h>
#include <assert.h>

#include "stream-reader.hpp"


StreamReader::StreamReader(const char *name, size_t ch_count)
	: m_name(name)
	, m_ch_count(ch_count)
	, m_frame_size(ch_count * sizeof(Sample))
{
}


StreamReader::~StreamReader()
{
}


void StreamReader::open(void)
{
	SDL_AudioSpec spec;
	spec.freq = 48000;
	spec.format = SDL_AUDIO_S16LE;
	spec.channels = m_ch_count;
	spec.freq = 48000;
	m_sdl_stream = do_open(&spec);
}


void StreamReader::poll(void)
{
	assert(m_sdl_stream != nullptr);
	int bytes_queued = SDL_GetAudioStreamQueued(m_sdl_stream);
	if(bytes_queued < 64000) {
		do_poll(m_sdl_stream);
	}
}


