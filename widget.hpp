#pragma once

#include <fftw3.h>
#include "stream.hpp"
#include "window.hpp"


class Widget {
	

public:
	enum Type : int {
		None, Spectrum, Waterfall, Waveform
	};

	Widget(Type type = None);
	void configure_fft(size_t size, Window::Type window_type);

	void draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_waveform(Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_spectrum(Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

private:
	Type m_type;
	bool m_channel_map[8];

	struct {
		int count;
		int step;
		bool agc;
		float peak;
	} m_waveform;

	struct {
		size_t size;
		Window::Type window_type;
		std::vector<float> in;
		std::vector<float> out;
		fftwf_plan plan;
		Window window;
	} m_spectrum;
};

