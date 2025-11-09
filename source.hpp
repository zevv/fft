
#pragma once

#include <stdint.h>
#include <assert.h>
#include <vector>

#include "types.hpp"
#include "misc.hpp"
#include "stream.hpp"

class Source {
public:

	struct Info {
		const char *name;
		const char *description;
		Source *(*fn_new)(SDL_AudioSpec &dst_spec, char *args);
	};

	Source(Source::Info &info, SDL_AudioSpec &dst_spec)
		: m_info(info)
		, m_frame_size(dst_spec.channels * sizeof(Sample))
		, m_dst_spec(dst_spec)
	{}

	virtual ~Source() {};
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
	Source::Info m_info;
	size_t m_frame_size{};
	SDL_AudioSpec m_dst_spec{};
	SDL_AudioStream *m_sdl_stream{};
};

