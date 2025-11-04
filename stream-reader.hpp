
#pragma once

#include <stdint.h>

#include "types.hpp"
#include "stream.hpp"

class StreamReader {
public:
	StreamReader(const char *name, size_t ch_count)
		: m_name(name)
		, m_ch_count(ch_count)
		, m_frame_size(ch_count * sizeof(Sample))
		, m_sdl_audio_spec{ SDL_AUDIO_S16LE, (uint8_t)ch_count, 48000 }
	{}

	virtual ~StreamReader() {};
	size_t channel_count() { return m_sdl_audio_spec.channels; }
	size_t frame_size() { return m_sdl_audio_spec.channels * sizeof(Sample); }
	const char *name() { return m_name; }
	SDL_AudioStream *get_sdl_audio_stream() { return m_sdl_stream; }

	virtual void open(void) = 0;
	virtual void poll(void) = 0;

protected:
	const char *m_name;
	size_t m_ch_count;
	size_t m_frame_size{};
	SDL_AudioSpec m_sdl_audio_spec{};
	SDL_AudioStream *m_sdl_stream{};
};


