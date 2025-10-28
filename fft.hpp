
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
	std::vector<int8_t> run(Sample *input, size_t stride=1);

private:
	fftwf_plan m_plan{nullptr};
	Window m_window{};
	std::vector<float> m_in;
	std::vector<int8_t> m_out;
	std::map<double, std::vector<int8_t>> m_cache;
};
