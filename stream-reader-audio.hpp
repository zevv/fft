
#include <SDL3/SDL_audio.h>

#include "stream-reader.hpp"


class StreamReaderAudio : public StreamReader {
public:
	StreamReaderAudio(size_t ch_count, float srate);
	~StreamReaderAudio();

	void do_poll(SDL_AudioStream *sas) override;
	SDL_AudioStream *do_open(SDL_AudioSpec *spec_dst) override;
private:
	SDL_AudioStream *m_sdl_audiostream{};
};

