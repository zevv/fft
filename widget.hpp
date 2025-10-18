#pragma once

#include <fftw3.h>
#include "stream.hpp"
#include "window.hpp"
#include "view.hpp"
#include "config.hpp"


class Widget {
	

public:
	enum Type : int {
		None, Spectrum, Waterfall, Waveform
	};

	Widget(Type type = None);

	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Widget *copy();

	void configure_fft(int size, Window::Type window_type);

	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_waveform(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_spectrum(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

	static const char* type_to_string(Type type);
	static Type string_to_type(const char *str);
	static const char **type_names();
	static size_t type_count();

private:
	Type m_type;
	bool m_channel_map[8];

	struct {
		bool agc;
		float peak;
	} m_waveform;

	struct {
		int size;
		Window::Type window_type;
		float window_beta;
		std::vector<float> in;
		std::vector<float> out;
		fftwf_plan plan;
		Window window;
	} m_spectrum;
};

