
#include <math.h>


#include "source.hpp"
#include "sourceregistry.hpp"

class SourceGenerator : public Source {
public:

	SourceGenerator(Source::Info &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceGenerator();

	void open() override;
	void poll() override;
private:

	void gen_sine(std::vector<Sample> &buf);
	void gen_saw(std::vector<Sample> &buf);
	void gen_sweep(std::vector<Sample> &buf);

	Samplerate m_srate{};
	Time m_phase{};
	double m_aux1{};
	int m_type{};
	std::vector<Sample> m_buf{};
};




SourceGenerator::SourceGenerator(Source::Info &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec, args)
	, m_srate(dst_spec.freq)
	, m_type(0)
{
	m_buf.resize(1024);
}



SourceGenerator::~SourceGenerator()
{
}


void SourceGenerator::open()
{
	m_sdl_stream = SDL_CreateAudioStream(&m_dst_spec, &m_dst_spec);
}


void SourceGenerator::gen_sine(std::vector<Sample> &buf)
{
	Time dp = 440.0 * 2.0 * M_PI / m_srate;
	for(size_t i=0; i<buf.size(); i++) {
		buf[i] = sin(m_phase) * k_sample_max;
		m_phase += dp;
		if(m_phase > 2.0 * M_PI) m_phase -= 2.0 * M_PI;
	}
}


void SourceGenerator::gen_saw(std::vector<Sample> &buf)
{
	Sample v = 0;

	if(m_type == 0) {
		v = sin(m_phase) * k_sample_max;
		m_phase += 4000.0 * 2.0 * M_PI / m_srate;
	}
}


void SourceGenerator::gen_sweep(std::vector<Sample> &buf)
{
	for(size_t i=0; i<buf.size(); i++) {
		buf[i] = sin(m_phase) * k_sample_max;
		m_phase += m_aux1 * 2.0 * M_PI / m_srate;
		m_aux1 += 0.1;
		if(m_phase > 2.0 * M_PI) m_phase -= 2.0 * M_PI;
	}
}

void SourceGenerator::poll()
{
	if(m_type == 0) gen_sine(m_buf);
	if(m_type == 1) gen_saw(m_buf);
	if(m_type == 2) gen_sweep(m_buf);

	SDL_PutAudioStreamData(m_sdl_stream, m_buf.data(), m_buf.size() * sizeof(Sample));
}


REGISTER_STREAM_READER(SourceGenerator,
	.name = "gen",
	.description = "signal generator",
);

