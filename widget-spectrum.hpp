#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "fft.hpp"

class Widget;

class Spectrum {
public:
	Spectrum();
	~Spectrum();
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	void copy_to(Spectrum &w);
	void draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
private:
	void configure_fft(int size, Window::Type window_type);

	Frequency x_to_freq(float x, SDL_Rect &r) {
		return m_freq_from + (m_freq_to - m_freq_from) * (x - r.x) / r.w;
	}

	float freq_to_x(Frequency freq, SDL_Rect &r) {
		return r.x + r.w * (freq - m_freq_from) / (m_freq_to - m_freq_from);
	}

	Frequency dx_to_dfreq(float dx, SDL_Rect &r) {
		return dx * (m_freq_to - m_freq_from) / r.w;
	}

	void pan(float delta) {
		m_freq_from += delta;
		m_freq_to += delta;
	};

	void zoom(float f) {
		m_freq_from += (m_freq_cursor - m_freq_from) * f;
		m_freq_to   -= (m_freq_to - m_freq_cursor) * f;
	};

	int m_size{256};
	Frequency m_freq_from{0.0};
	Frequency m_freq_to{1.0};
	Frequency m_freq_cursor{0.0};
	float m_amp_cursor{0.0};
	Window::Type m_window_type{Window::Type::Hanning};
	float m_window_beta{5.0f};
	Fft m_fft{};

	std::vector<Sample> m_in;
	std::vector<Sample> m_out_graph;
};

