
#include <math.h>


#include "source.hpp"
#include "sourceregistry.hpp"
#include "biquad.hpp"

#include <fftw3.h>

#include "window.hpp"


static SDL_AudioStream *g_sdl_stream = nullptr;

class SourceDebug : public Source {
public:

	SourceDebug(Source::Info &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceDebug();

	void open() override;

private:
	SDL_AudioSpec m_src_spec;
};




SourceDebug::SourceDebug(Source::Info &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec, args)
{
	m_src_spec = sdl_audiospec_from_str(args);
	m_dst_spec.channels = m_src_spec.channels;
}


SourceDebug::~SourceDebug()
{
}


void SourceDebug::open()
{
	m_sdl_stream = SDL_CreateAudioStream(&m_src_spec, &m_dst_spec);
	g_sdl_stream = m_sdl_stream;
}


void debug_source_put(float *data, size_t len)
{
	if (g_sdl_stream) {
		SDL_PutAudioStreamData(g_sdl_stream, (void *)data, len * sizeof(float));
	}
}



REGISTER_STREAM_READER(SourceDebug,
	.name = "debug",
	.description = "debug generator",
);

