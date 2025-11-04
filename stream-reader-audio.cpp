
#include "stream-reader-audio.hpp"


StreamReaderAudio::StreamReaderAudio(size_t ch_count, float srate)
	: StreamReader("audio", ch_count)
{
}


StreamReaderAudio::~StreamReaderAudio()
{
}


void StreamReaderAudio::open()
{
	m_sdl_stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &m_sdl_audio_spec, nullptr, (void *)this);

	if(m_sdl_stream) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_stream));
	} else {
		fprintf(stderr, "StreamReaderAudio: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
	}
}



void StreamReaderAudio::poll()
{
}


