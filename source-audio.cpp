
#include "source.hpp"
#include "sourceregistry.hpp"


class SourceAudio : public Source {
public:
	SourceAudio(Source::Info &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceAudio();

	void open() override;
	void resume() override;
	void pause() override;
};



SourceAudio::SourceAudio(Source::Info &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec)
{
	m_dst_spec = sdl_audiospec_from_str(args);
}


SourceAudio::~SourceAudio()
{
}


void SourceAudio::open()
{
	m_sdl_stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &m_dst_spec, nullptr, (void *)this);

	if(m_sdl_stream) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_stream));
	} else {
		fprintf(stderr, "SourceAudio: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
	}
}



void SourceAudio::pause()
{
	if(m_sdl_stream) {
		SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(m_sdl_stream));
	}
}


void SourceAudio::resume()
{
	if(m_sdl_stream) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_stream));
	}
}


REGISTER_STREAM_READER(SourceAudio,
	.name = "audio",
	.description = "SDL audio source",
);

