
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
	, m_type(atoi(args))
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


static Sample float_to_s16(double v)
{
	double dither = (double)rand() / RAND_MAX - 0.5;
	return (int16_t)floor(v * 32767 + dither);
}


void SourceGenerator::gen_sine(std::vector<Sample> &buf)
{
	for(size_t i=0; i<buf.size(); i++) {
		buf[i] = float_to_s16(sin(m_phase * 2 * M_PI));
		m_phase += 440.0 / m_srate;
		m_phase = fmod(m_phase, 1.0);
	}
}


void SourceGenerator::gen_saw(std::vector<Sample> &buf)
{
	Sample v = 0;

	if(m_type == 0) {
		v = float_to_s16(sin(m_phase * 2 * M_PI));
		m_phase += 440.0 / m_srate;
		m_phase = fmod(m_phase, 1.0);
	}
}


void SourceGenerator::gen_sweep(std::vector<Sample> &buf)
{
	for(size_t i=0; i<buf.size(); i++) {
		buf[i] = float_to_s16(sin(m_phase * 2 * M_PI));
		m_aux1 += 0.1;
		m_phase += m_aux1 / m_srate;
		m_phase = fmod(m_phase, 1.0);
		
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

