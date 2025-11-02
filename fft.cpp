#include <assert.h>
#include <algorithm>

#include "fft.hpp"

Fft::Fft(bool approximate)
	: m_approximate(approximate)
{
	configure(1024, Window::Type::Hanning, 0.0f);
}


Fft::~Fft()
{
	if(m_plan) fftwf_destroy_plan(m_plan);
	if(m_in) fftwf_free(m_in);
}


void Fft::configure(size_t size, Window::Type window_type, float window_beta)
{
	if(m_window.size() != size || m_window.type() != window_type || m_window.beta() != window_beta) {
		m_window.configure(window_type, size, window_beta);
	}

	if(m_size != size) {
		m_size = size;
		if(m_in) free(m_in);
		m_in = (float*)fftwf_malloc(sizeof(float) * size);
		m_out.resize(size / 2 + 1);
		if(m_plan) fftwf_destroy_plan(m_plan);
		m_plan = fftwf_plan_r2r_1d(size, m_in, m_in, FFTW_R2HC, FFTW_ESTIMATE);
	}
}


int Fft::out_size()
{
	return m_out.size();
}


static inline float real_20log10(float v)
{
	float vout = 20.0f * log10f(v + 1e-10f);
	return std::clamp(vout, -127.0f, 0.0f);
}


static inline float fast_20log10(float v)
{
	int32_t i_v = std::bit_cast<int32_t>(v + 1e-10f);
	float vout = (float)i_v * 7.17897e-7f - 764.616f;
	return std::clamp(vout, -127.0f, 0.0f);
}


static inline float fast_hypot(float x, float y)
{
	float vmax = std::max(fabs(x), fabs(y));
	float vmin = std::min(fabs(x), fabs(y));
	return vmax + vmin * (0.5f + (vmin * 0.25f) / (vmax + 1e-10f));	
}


std::vector<int8_t> Fft::run(Sample *input, size_t stride)
{
	auto window = m_window.data();
	for(size_t i=0; i<m_size; i++) {
		m_in[i] = input[i * stride] * window[i] / k_sample_max;
	}
	fftwf_execute(m_plan);

	float scale = m_window.gain() * 2.0f / m_size;

	if(m_approximate) {
		m_out[0] = fast_20log10(fabs(m_in[0]) * scale / 2);
		for(size_t i=1; i<m_size/2; i++) {
			m_out[i] = fast_20log10(fast_hypot(m_in[i], m_in[m_size - i]) * scale);
		}
		m_out[m_size / 2] = fast_20log10(fabs(m_in[m_size / 2]) * scale / 2);
	} else {
		m_out[0] = real_20log10(fabs(m_in[0]) * scale / 2);
		for(size_t i=1; i<m_size/2; i++) {
			m_out[i] = real_20log10(hypot(m_in[i], m_in[m_size - i]) * scale);
		}
		m_out[m_size / 2] = real_20log10(fabs(m_in[m_size / 2]) * scale / 2);
	}


	return m_out;
}


