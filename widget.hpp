#pragma once

#include <fftw3.h>
#include "stream.hpp"
#include "window.hpp"
#include "view.hpp"
#include "config.hpp"


class Widget {
	

public:
	enum Type : int {
		None, Waveform, Spectrum, Waterfall
	};

	Widget(Type type = None);
	~Widget();

	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Widget *copy();

	void configure_fft(int size, Window::Type window_type);

	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_waveform(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_spectrum(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	bool has_focus() const { return m_has_focus; }

	static const char* type_to_string(Type type);
	static Type string_to_type(const char *str);
	static const char **type_names();
	static size_t type_count();

private:

	ImVec4 channel_color(int channel);
	float draw_graph(View &view, SDL_Renderer *rend, SDL_Rect &r, 
					 ImVec4 &col, float *data, size_t stride,
					 int idx_from, int idx_to,
					 float y_min, float y_max);

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

	bool m_has_focus;
};

