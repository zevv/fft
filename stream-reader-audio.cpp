
#include "stream-reader-audio.hpp"


StreamReaderAudio::StreamReaderAudio(SDL_AudioSpec &dst_spec)
	: StreamReader("audio", dst_spec)
{
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


