
#include "stream-reader.hpp"


class StreamReaderAudio : public StreamReader {
public:
	StreamReaderAudio(StreamReaderInfo &info, SDL_AudioSpec &dst_spec, char *args);
	~StreamReaderAudio();

	void open() override;
	void resume() override;
	void pause() override;
};



StreamReaderAudio::StreamReaderAudio(StreamReaderInfo &info, SDL_AudioSpec &dst_spec, char *args)
	: StreamReader(info, dst_spec)
{
	m_dst_spec = sdl_audiospec_from_str(args);
}


StreamReaderAudio::~StreamReaderAudio()
{
}


void StreamReaderAudio::open()
{
	m_sdl_stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &m_dst_spec, nullptr, (void *)this);

	if(m_sdl_stream) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_stream));
	} else {
		fprintf(stderr, "StreamReaderAudio: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
	}
}



void StreamReaderAudio::pause()
{
	if(m_sdl_stream) {
		SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(m_sdl_stream));
	}
}


void StreamReaderAudio::resume()
{
	if(m_sdl_stream) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_stream));
	}
}


REGISTER_STREAM_READER(StreamReaderAudio,
	.name = "audio",
	.description = "SDL audio source",
);

