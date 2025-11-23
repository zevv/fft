
#include <math.h>


#include "source.hpp"
#include "sourceregistry.hpp"
#include "biquad.hpp"

#include <fftw3.h>

#include "window.hpp"


class FirFilter {
public:
	enum class Type { LP };

	FirFilter(size_t size)
		: m_size(size)
	{
		m_buf.resize(size);
		m_fft_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * size);
		m_kernel.resize(size);
		m_plan =fftw_plan_dft_c2r_1d(size, m_fft_in, m_kernel.data(), FFTW_ESTIMATE);
		m_window.configure(Window::Type::Hanning, size);
	}

	~FirFilter()
	{
		fftw_destroy_plan(m_plan);
		fftw_free(m_fft_in);
	}

	void configure(Type type, Frequency f_cut)
	{
		// create complex spectrum
		double delay = (double)(m_size - 1) / 2.0;
		for(size_t i=0; i<m_size/2+1; i++) {
			double mag = 0.0;
			double f = (double)i / (double)m_size;
			if(type == Type::LP) {
				mag = (f < f_cut) ? 1.0 : 0.0;
			}
			double pha = -2.0 * M_PI * ((double)i * delay / (double)m_size);
			m_fft_in[i][0] = mag * cos(pha);
			m_fft_in[i][1] = mag * sin(pha);
		}

		// convert to time domain
		fftw_execute(m_plan);

		// windowing and scaling
		auto wdata = m_window.data();
		double gain = 1.0 / m_size;
		for(size_t i=0; i<m_size; i++) {
			m_kernel[i] *= wdata[i] * gain;
		}
	}
	
	double run(double v)
	{
		m_buf[m_buf_head] = v;
		m_buf_head = (m_buf_head + 1) % m_size;

		double out = 0.0;
		for(size_t i=0; i<m_size; i++) {
			size_t idx = (m_buf_head + i) % m_size;
			out += m_buf[idx] * m_kernel[i];
		}
		return out;
	}


private:
	size_t m_size{};
	fftw_plan m_plan{};
	fftw_complex *m_fft_in{};
	std::vector<double> m_kernel{};
	std::vector<double> m_buf{};
	size_t m_buf_head{};
	Window m_window{};
};




static SDL_AudioStream *g_sdl_stream = nullptr;


class SourceDebug : public Source {
public:

	SourceDebug(Source::Info &info, SDL_AudioSpec &dst_spec, char *args);
	~SourceDebug();

	void open() override;

};




SourceDebug::SourceDebug(Source::Info &info, SDL_AudioSpec &dst_spec, char *args)
	: Source(info, dst_spec, args)
{
}


SourceDebug::~SourceDebug()
{
}


void SourceDebug::open()
{
	m_sdl_stream = SDL_CreateAudioStream(&m_dst_spec, &m_dst_spec);
	g_sdl_stream = m_sdl_stream;
}


void debug_source_put(void *data, Uint8 *stream, int len)
{
	if (g_sdl_stream) {
		SDL_PutAudioStreamData(g_sdl_stream, data, len);
	}
}



REGISTER_STREAM_READER(SourceDebug,
	.name = "debug",
	.description = "debug generator",
);

