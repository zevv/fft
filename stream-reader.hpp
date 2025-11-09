
#pragma once

#include <stdint.h>
#include <assert.h>
#include <vector>

#include "types.hpp"
#include "misc.hpp"
#include "stream.hpp"

class StreamReader;

struct StreamReaderInfo {
	const char *name;
	const char *description;
	StreamReader *(*fn_create)(SDL_AudioSpec &dst_spec, char *args);
};

class StreamReaderReg {
public:
	StreamReaderReg(StreamReaderInfo info);
	static StreamReader *create(const char *name, SDL_AudioSpec &dst_spec, char *args);
};


class StreamReader {
public:
	StreamReader(StreamReaderInfo &info, SDL_AudioSpec &dst_spec)
		: m_info(info)
		, m_frame_size(dst_spec.channels * sizeof(Sample))
		, m_dst_spec(dst_spec)
	{}

	virtual ~StreamReader() {};
	size_t channel_count() { return m_dst_spec.channels; }
	size_t frame_size() { return m_dst_spec.channels * sizeof(Sample); }
	const char *name() { return m_info.name; }
	SDL_AudioStream *get_sdl_audio_stream() { return m_sdl_stream; }
	void dump(FILE *f) {
		fprintf(f, "%s (%s) %d/%s/%d\n",
				m_info.name,
				m_info.description,
				(int)m_dst_spec.channels,
				sdl_audioformat_to_str(m_dst_spec.format),
				(int)m_dst_spec.freq);
	}

	virtual void open(void) = 0;
	virtual void poll(void) {};
	virtual void pause(void) {};
	virtual void resume(void) {};

protected:
	StreamReaderInfo m_info;
	size_t m_frame_size{};
	SDL_AudioSpec m_dst_spec{};
	SDL_AudioStream *m_sdl_stream{};
};


#define REGISTER_STREAM_READER(class, ...) \
	static StreamReaderInfo reg = { \
		__VA_ARGS__ \
		.fn_create = [](SDL_AudioSpec &dst_spec, char *args) -> StreamReader* { return new class(reg, dst_spec, args); }, \
	}; \
	static StreamReaderReg info(reg);
