#include <assert.h>

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
	}

	if(m_in.size() != size) {
		m_in.resize(size);
		m_out.resize(size / 2 + 1);

		if(m_plan) fftwf_destroy_plan(m_plan);
		m_plan = fftwf_plan_r2r_1d(size, m_in.data(), m_in.data(), FFTW_R2HC, FFTW_ESTIMATE);

		m_in2.resize(8 * size);
		m_out2.resize(8 * (size / 2 + 1));

		if(m_plan2) fftwf_destroy_plan(m_plan2);
		int rank = 1;
		int n[8] = { (int)size, (int)size, (int)size, (int)size,
		             (int)size, (int)size, (int)size, (int)size };
		int idist = 1;
		int istride = 8;
		int odist = 1;
		int ostride = 8;
		fftw_r2r_kind kind[8] = { FFTW_R2HC, FFTW_R2HC, FFTW_R2HC, FFTW_R2HC,
		                         FFTW_R2HC, FFTW_R2HC, FFTW_R2HC, FFTW_R2HC };
		printf("%d\n", fftwf_planner_nthreads());
		m_plan2 = fftwf_plan_many_r2r(rank, n, 8,
									  m_in2.data(), n,
									  istride, idist,
									  m_in2.data(), n,
									  ostride, odist,
									  kind, FFTW_ESTIMATE);
	}
}


int Fft::out_size()
{
	return m_out.size();
}


std::vector<float> Fft::run(Sample *input, size_t stride)
{
	double key = 0.0;
	for(size_t i=0; i<m_in.size(); i++) {
		key += input[i * stride] * i;
	}

	auto it = m_cache.find(key);
	if(it != m_cache.end()) {
		return it->second;
	}

	size_t size = m_in.size();

	auto window = m_window.data();
	for(size_t i=0; i<size; i++) {
		float scale = window[i] / k_sample_max;
		m_in[i] = input[i * stride] * scale;
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
		m_out[i] = (v >= 1e-20f) ? 20.0f * log10f(v) : db_range;
	}

	//m_cache[key] = m_out;

	return m_out;
}


std::vector<float> Fft::run_many(Sample *input, size_t stride)
{
	size_t size = m_in.size();

	float *p_windowed = m_in2.data();
	auto window = m_window.data();
	for(size_t i=0; i<size; i++) {
		float w = window[i] / k_sample_max;
		for(size_t ch=0; ch<8; ch++) {
			*p_windowed++ = input[i * stride + ch] * w;
		}
	}

	fftwf_execute(m_plan2);

	float scale = m_window.gain() * 2.0f / size;
	float db_range = -120.0;
	float *pout = m_out2.data();

	for(size_t i=0; i<=size/2; i++) {
		if(i == 0) {
			for(size_t ch=0; ch<8; ch++) {
				float v = m_in2[ch] * scale / 2;
				*pout++ = (v >= 1e-20f) ? 20.0f * log10f(v) : db_range;
			}
		} else if(i == size / 2) {
			for(size_t ch=0; ch<8; ch++) {
				float v = fabs(m_in2[(size / 2) * 8 + ch]) * scale / 2;
				*pout++ = (v >= 1e-20f) ? 20.0f * log10f(v) : db_range;
			}
		} else {
			for(size_t ch=0; ch<8; ch++) {
				float v = hypot(m_in2[i * 8 + ch], m_in2[(size - i) * 8 + ch]) * scale;
				*pout++ = (v >= 1e-20f) ? 20.0f * log10f(v) : db_range;
			}
		}
	}

	return m_out2;
}

