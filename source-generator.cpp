
#include <math.h>


#include "source.hpp"
#include "sourceregistry.hpp"

class SourceGenerator : public Source {
public:

	SourceGenerator(SourceInfo &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceGenerator();

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




SourceGenerator::SourceGenerator(SourceInfo &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec)
	, m_srate(dst_spec.freq)
	, m_type(0)
{
	m_buf.resize(dst_spec.channels * 1024);
}



SourceGenerator::~SourceGenerator()
{
}


void SourceGenerator::open()
{
	m_sdl_stream = SDL_CreateAudioStream(&m_dst_spec, &m_dst_spec);
}


void SourceGenerator::poll()
{
	for(size_t i=0; i<1024; i++) {
		m_buf[i] = run();
	}
	SDL_PutAudioStreamData(m_sdl_stream, m_buf.data(), 1024 * sizeof(Sample));
}


Sample SourceGenerator::run()
{
	Sample v = 0;

	if(m_type == 0) {
		v = sin(m_phase) * k_sample_max;
		m_phase += 440.0 * 2.0 * M_PI / m_srate;
	}

	if(m_type == 1) {
		v = (m_phase - 0.5) * k_sample_max;
		m_phase += 440.0 / m_srate;
	}
	
	if(m_type == 2) {
		v = sin(m_phase) * k_sample_max;
		m_phase += m_aux1 * 2.0 * M_PI / m_srate;
		m_aux1 += 0.1;
	}
		
	return v;
}
