
#include <math.h>

#include "stream-reader-generator.hpp"


StreamReaderGenerator::StreamReaderGenerator(size_t ch_count, float srate, int type)
	: StreamReader("gen", ch_count)
	, m_srate(srate)
	, m_type(type)
{
}



StreamReaderGenerator::~StreamReaderGenerator()
{
}


void StreamReaderGenerator::poll()
{
	size_t bytes_gen;
	Sample *buf = (Sample *)m_rb.get_write_ptr(&bytes_gen);
	size_t frames_gen = bytes_gen / m_frame_size;
	if(frames_gen == 0) return;

	for(size_t i=0; i<frames_gen; i++) {
		buf[i] = run();
	}
	m_rb.write_done(frames_gen * m_frame_size);
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
