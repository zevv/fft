
#pragma once

#include <fftw3.h>
#include <math.h>
#include <map>

#include "window.hpp"

class Fft {

public:

	Fft();
	~Fft();

	void set_size(size_t size);
	void set_window(Window::Type type, size_t size, float beta=5.0f);
	int out_size();
	std::vector<float> run(std::vector<Sample> &input);

private:
	fftwf_plan m_plan{nullptr};
	Window m_window{};
	std::vector<float> m_in;
	std::vector<float> m_out;
	std::map<double, std::vector<float>> m_cache;
};
