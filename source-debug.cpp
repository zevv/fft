
#include <math.h>


#include "source.hpp"
#include "sourceregistry.hpp"
#include "biquad.hpp"

static SDL_AudioStream *g_sdl_stream = nullptr;


class SourceDebug : public Source {
public:

	SourceDebug(Source::Info &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceDebug();

	void open() override;

};




SourceDebug::SourceDebug(Source::Info &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec, args)
{
}


SourceDebug::~SourceDebug()
{
}


void SourceDebug::open()
{
	m_sdl_stream = SDL_CreateAudioStream(&m_dst_spec, &m_dst_spec);
	g_sdl_stream = m_sdl_stream;
}


void debug_source_put(void *data, Uint8 *stream, int len)
{
	if (g_sdl_stream) {
		SDL_PutAudioStreamData(g_sdl_stream, data, len);
	}
}



REGISTER_STREAM_READER(SourceDebug,
	.name = "debug",
	.description = "debug generator",
);

