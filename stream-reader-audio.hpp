
#include <SDL3/SDL_audio.h>

#include "stream-reader.hpp"


class StreamReaderAudio : public StreamReader {
public:
	StreamReaderAudio(size_t ch_count, float srate);
	~StreamReaderAudio();

	void open() override;
	void poll() override;
};

