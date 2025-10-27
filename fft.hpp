
#pragma once

#include <fftw3.h>
#include <math.h>
#include <map>

#include "window.hpp"

class Fft {

public:

	Fft();
	~Fft();

	void configure(size_t size, Window::Type type, float beta=5.0f);
	int out_size();
	std::vector<float> run(Sample *input, size_t stride=1);
	std::vector<float> run_many(Sample *input, size_t stride);

private:
	fftwf_plan m_plan{nullptr};
	Window m_window{};
	std::vector<float> m_in;
	std::vector<float> m_out;
	
	fftwf_plan m_plan2{nullptr};
	std::vector<float> m_in2;
	std::vector<float> m_out2;

	std::map<double, std::vector<float>> m_cache;
};
