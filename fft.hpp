
#pragma once

#include <fftw3.h>
#include <math.h>
#include <map>

#include "window.hpp"

class Fft {

public:

	Fft(bool approximate=false);
	~Fft();

	void configure(size_t size, Window::Type type, float beta=5.0f);
	int out_size();
	std::vector<int8_t> run(Sample *input, size_t stride=1);
	void set_approximate(bool v) { m_approximate = v; }

private:
	bool m_approximate{false};
	fftwf_plan m_plan{nullptr};
	Window m_window{};
	size_t m_size{};
	float *m_in{};
	std::vector<float> m_outf{};
	std::vector<int8_t> m_out{};
};
