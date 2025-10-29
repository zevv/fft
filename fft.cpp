#include <assert.h>
#include <algorithm>

#include "fft.hpp"

Fft::Fft()
{
	configure(1024, Window::Type::Hanning, 0.0f);
}


Fft::~Fft()
{
	if(m_plan) {
		fftwf_destroy_plan(m_plan);
	}
}


void Fft::configure(size_t size, Window::Type window_type, float window_beta)
{
	if(m_window.size() != size || m_window.type() != window_type || m_window.beta() != window_beta) {
		m_window.configure(window_type, size, window_beta);
		m_cache.clear();
	}

	if(m_in.size() != size) {
		m_in.resize(size);
		m_out.resize(size / 2 + 1);
		if(m_plan) fftwf_destroy_plan(m_plan);
		m_plan = fftwf_plan_r2r_1d(size, m_in.data(), m_in.data(), FFTW_R2HC, FFTW_ESTIMATE);
		m_cache.clear();
	}
}


int Fft::out_size()
{
	return m_out.size();
}


std::vector<int8_t> Fft::run(Sample *input, size_t stride)
{
	double key = 0.0;
	size_t size = m_in.size();
	for(size_t i=0; i<size; i++) {
		key += static_cast<double>(input[i * stride]) * 1e-6;
	}

	auto it = m_cache.find(key);
	if(it != m_cache.end()) {
		return it->second;
	}

	auto window = m_window.data();
	for(size_t i=0; i<size; i++) {
		m_in[i] = input[i * stride] * window[i] / k_sample_max;
	}
	fftwf_execute(m_plan);

	float scale = m_window.gain() * 2.0f / size;
	float db_range = -120.0;

	for(size_t i=0; i<=size/2; i++) {
		float v = 0.0;
		if(i == 0) {
			v = m_in[0] * scale / 2;
		} else if(i == size / 2) {
			v = fabs(m_in[size / 2]) * scale / 2;
		} else {
			v = hypot(m_in[i], m_in[size - i]) * scale;
		} 
		m_out[i] = std::clamp(20.0f * log10f(v + 1e-10), -127.0f, 0.0f);
	}

	//m_cache[key] = m_out;

	return m_out;
}


