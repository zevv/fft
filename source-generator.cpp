
#include <math.h>


#include "source.hpp"
#include "sourceregistry.hpp"
#include "biquad.hpp"

class SourceGenerator : public Source {
public:

	SourceGenerator(Source::Info &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceGenerator();

	void open() override;
	void poll() override;
	void draw() override;

private:

	void gen_sine(std::vector<Sample> &buf);
	void gen_saw(std::vector<Sample> &buf);
	void gen_sweep(std::vector<Sample> &buf);
	void gen_noise(std::vector<Sample> &buf);

	Samplerate m_srate{};
	Time m_phase{};
	double m_aux1{};
	int m_type{};
	std::vector<Sample> m_buf{};

	struct {
		bool enabled{false};
		Frequency freq{1000.0};
		double Q{0.707};
		Biquad bq[2]{};
	} m_filter;
};




SourceGenerator::SourceGenerator(Source::Info &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec, args)
	, m_srate(dst_spec.freq)
	, m_type(atoi(args))
{
	m_buf.resize(1024);
	m_filter.bq[0].configure(Biquad::Type::LP, 0.3);
	m_filter.bq[1].configure(Biquad::Type::LP, 0.3);
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
	if(m_type == 0) gen_sine(m_buf);
	if(m_type == 1) gen_saw(m_buf);
	if(m_type == 2) gen_sweep(m_buf);
	if(m_type == 3) gen_noise(m_buf);

	if(m_filter.enabled) {
		for(size_t i=0; i<m_buf.size(); i++) {
			m_buf[i] = m_filter.bq[1].run(m_filter.bq[0].run(m_buf[i]));
		}
	}

	SDL_PutAudioStreamData(m_sdl_stream, m_buf.data(), m_buf.size() * sizeof(Sample));
}


void SourceGenerator::draw()
{
	bool update = false;

	if(ImGui::ToggleButton("LP", &m_filter.enabled)) {
		update = true;

	}

	if(m_filter.enabled) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		if(ImGui::SliderDouble("##filter freq", &m_filter.freq, 20.0, m_srate / 2.0, "f: %.0f Hz")) {
			update = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		if(ImGui::SliderDouble("##filter Q", &m_filter.Q, 0.1, 10.0, "Q: %.2f")) {
			update = true;
		}

		if(update) {
			m_filter.bq[0].configure(Biquad::Type::LP, 2.0 * m_filter.freq / m_srate, m_filter.Q);
			m_filter.bq[1].configure(Biquad::Type::LP, 2.0 * m_filter.freq / m_srate, m_filter.Q);
			SDL_ClearAudioStream(m_sdl_stream);
		}
	}
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


void SourceGenerator::gen_noise(std::vector<Sample> &buf)
{
	for(size_t i=0; i<buf.size(); i++) {
		float v = (double)rand() / RAND_MAX * 2.0 - 1.0;
		buf[i] = float_to_s16(v * 0.01);
	}
}

REGISTER_STREAM_READER(SourceGenerator,
	.name = "gen",
	.description = "signal generator",
);

