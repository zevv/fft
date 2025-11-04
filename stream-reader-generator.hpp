
#include "stream-reader.hpp"

class StreamReaderGenerator : public StreamReader {
public:

	StreamReaderGenerator(size_t ch_count, float srate, int type);
	~StreamReaderGenerator();

	void do_poll(SDL_AudioStream *sas) override;
private:

	Sample run();
	Samplerate m_srate{};
	Time m_phase{};
	int m_type{};
};


