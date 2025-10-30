
#include <SDL3/SDL_audio.h>

#include "stream-reader.hpp"


class StreamReaderAudio : public StreamReader {
public:
	StreamReaderAudio(size_t ch_count, float srate);
	~StreamReaderAudio();
	void poll() override;
private:
	SDL_AudioStream *m_sdl_audiostream{};
};

