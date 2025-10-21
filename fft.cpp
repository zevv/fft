#include <assert.h>

#include "fft.hpp"

Fft::Fft()
{
}


Fft::~Fft()
{
	if(m_plan) {
		fftwf_destroy_plan(m_plan);
	}
}


void Fft::set_size(size_t size)
{
	if(m_plan) {
		fftwf_destroy_plan(m_plan);
	}

	m_in.resize(size);
	m_out.resize(size);
	m_plan = fftwf_plan_r2r_1d(size, m_in.data(), m_in.data(), FFTW_R2HC, FFTW_ESTIMATE);
	m_window.set_size(size);
}


void Fft::set_window(Window::Type type, size_t size, float beta)
{
	m_window.configure(type, size, beta);
}


int Fft::out_size()
{
	return m_out.size();
}


std::vector<Sample> &Fft::run(std::vector<Sample> &input)
{
	assert(m_in.size() == input.size());
	assert(m_window.size() == input.size());

	size_t size = m_in.size();

	auto window = m_window.data();
	for(size_t i=0; i<size; i++) {
		m_in[i] = input[i] * window[i];
	}
	fftwf_execute(m_plan);

	float scale = 2.0f / size;
	float db_range = -120.0;

	for(size_t i=0; i<size; i++) {
		Sample v = 0.0;
		if(i == 0) {
			v = m_in[0] * scale / 2;
		} else if(i < size / 2) {
			v = hypot(m_in[i], m_in[size - i]) * scale;
		} else if(i == size / 2) {
			v = fabs(m_in[size / 2]) * scale / 2;
		} 
		m_out[i] = (v >= 1e-20f) ? 20.0f * log10f(v) : db_range;
	}

	return m_out;
}


