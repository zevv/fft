
#include "stream-reader-audio.hpp"


StreamReaderAudio::StreamReaderAudio(size_t ch_count, float srate)
	: StreamReader("audio", ch_count)
{
	SDL_AudioSpec fmt{};
    fmt.freq = srate;
#ifdef SAMPLE_S16
	fmt.format = SDL_AUDIO_S16;
#else
    fmt.format = SDL_AUDIO_F32;
#endif
    fmt.channels = ch_count;

	SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &fmt, nullptr, (void *)this);

	if(stream) {
		m_sdl_audiostream = stream;
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
	} else {
		fprintf(stderr, "StreamReaderAudio: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
	}
}


StreamReaderAudio::~StreamReaderAudio()
{
	if(m_sdl_audiostream) {
		SDL_DestroyAudioStream(m_sdl_audiostream);
	}
	m_sdl_audiostream = nullptr;
}


void StreamReaderAudio::poll()
{
	if(!m_sdl_audiostream) return;
	size_t read_max;
	void *buf = m_rb.get_write_ptr(&read_max);
	if(read_max == 0) return;

	size_t bytes_per_frame = m_ch_count * sizeof(Sample);
	size_t frames_max = read_max / bytes_per_frame;
	size_t bytes_to_read = frames_max * bytes_per_frame;

	int r = SDL_GetAudioStreamData(m_sdl_audiostream, buf, bytes_to_read);
	if(r > 0) {
		m_rb.write_done(r);
	}
}


