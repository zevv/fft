
#include <math.h>

#include "stream-reader-generator.hpp"


StreamReaderGenerator::StreamReaderGenerator(size_t ch_count, float srate, int type)
	: StreamReader("gen", ch_count)
	, m_srate(srate)
	, m_type(type)
{
	m_buf.resize(ch_count * 1024);
}



StreamReaderGenerator::~StreamReaderGenerator()
{
}


void StreamReaderGenerator::open()
{
	m_sdl_stream = SDL_CreateAudioStream(&m_sdl_audio_spec, &m_sdl_audio_spec);
}


void StreamReaderGenerator::poll()
{
	for(size_t i=0; i<1024; i++) {
		m_buf[i] = run();
	}
	SDL_PutAudioStreamData(m_sdl_stream, m_buf.data(), 1024 * sizeof(Sample));
}


Sample StreamReaderGenerator::run()
{
	Sample v = 0;

	if(m_type == 0) {
		v = sin(2.0 * M_PI * 440.0 * m_phase) * k_sample_max;
		m_phase += 1.0 / m_srate;
		if(m_phase >= 1.0) m_phase -= 1.0;
	}

	if(m_type == 1) {
		v = (m_phase - 0.5) * k_sample_max;
		m_phase += 440.0 / m_srate;
		if(m_phase > 1.0) m_phase -= 1.0;
	}

	return v;
}
