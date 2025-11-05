
#include <SDL3/SDL_audio.h>

#include "stream-reader.hpp"


class StreamReaderAudio : public StreamReader {
public:
	StreamReaderAudio(SDL_AudioSpec &dst_spec);
	~StreamReaderAudio();

	void open() override;
	void poll() override;
};

