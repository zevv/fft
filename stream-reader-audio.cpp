
#include "stream-reader-audio.hpp"


StreamReaderAudio::StreamReaderAudio(size_t ch_count, float srate)
	: StreamReader("audio", ch_count)
{
}


SDL_AudioStream *StreamReaderAudio::do_open(SDL_AudioSpec *spec_dst)
{
	SDL_AudioStream *sas = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            spec_dst, nullptr, (void *)this);

	if(sas) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(sas));
	} else {
		fprintf(stderr, "StreamReaderAudio: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
	}

	return sas;
}


StreamReaderAudio::~StreamReaderAudio()
{
}


void StreamReaderAudio::do_poll(SDL_AudioStream *sas)
{
}


