
#include "stream-reader.hpp"

class StreamReaderGenerator : public StreamReader {
public:

	StreamReaderGenerator(SDL_AudioSpec &dst_spec, int type);
	~StreamReaderGenerator();

	void open() override;
	void poll() override;
private:

	Sample run();
	Samplerate m_srate{};
	Time m_phase{};
	double m_aux1{};
	int m_type{};
	std::vector<Sample> m_buf{};
};


