
#pragma once

#include <stdint.h>

#include "types.hpp"
#include "stream.hpp"

class StreamReader {
public:
	StreamReader(const char *name, SDL_AudioSpec &dst_spec)
		: m_name(name)
		, m_frame_size(dst_spec.channels * sizeof(Sample))
		, m_dst_spec(dst_spec)
	{}

	virtual ~StreamReader() {};
	size_t channel_count() { return m_dst_spec.channels; }
	size_t frame_size() { return m_dst_spec.channels * sizeof(Sample); }
	const char *name() { return m_name; }
	SDL_AudioStream *get_sdl_audio_stream() { return m_sdl_stream; }

	virtual void open(void) = 0;
	virtual void poll(void) = 0;

protected:
	const char *m_name;
	size_t m_frame_size{};
	SDL_AudioSpec m_dst_spec{};
	SDL_AudioStream *m_sdl_stream{};
};


