
#pragma once

#include <stdint.h>

#include "types.hpp"
#include "stream.hpp"

class StreamReader {
public:
	StreamReader(const char *name, size_t ch_count);
	virtual ~StreamReader();
	size_t channel_count() { return m_ch_count; }
	size_t frame_size() { return m_frame_size; }
	const char *name() { return m_name; }
	SDL_AudioStream *get_sdl_audio_stream() { return m_sdl_stream; }
	void open(void);
	void poll(void);

	virtual SDL_AudioStream *do_open(SDL_AudioSpec *spec) = 0;
	virtual void do_poll(SDL_AudioStream *sas) = 0;

protected:
	const char *m_name;
	size_t m_ch_count;
	size_t m_frame_size{};
	SDL_AudioStream *m_sdl_stream{};
};


