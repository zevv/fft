#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "fft.hpp"

class Widget;

class Waterfall {
public:
	Waterfall();
	~Waterfall();
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	void copy_to(Waterfall &w);
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

	void pan(float dx, float dy) {
		m_freq_from += dx;
		m_freq_to += dx;
		m_t_from += dy;
        m_t_to += dy;

	};

	void zoom(float fx, float fy) {
		m_freq_from += (m_freq_cursor - m_freq_from) * fx;
		m_freq_to   -= (m_freq_to - m_freq_cursor) * fx;
		m_t_from += (m_t_cursor - m_t_from) * fy;
        m_t_to   -= (m_t_to - m_t_cursor) * fy;

	};

	Time y_to_t(float y, SDL_Rect &r) {
		return m_t_from - (m_t_from - m_t_to) * (y - r.y) / r.h;
	}

	float t_to_y(Time t, SDL_Rect &r) {
		return r.y + r.h * (m_t_from - t) / (m_t_from - m_t_to);
	}

	Time dy_to_dt(float dy, SDL_Rect &r) {
		return dy * (m_t_to - m_t_from) / r.h;
	}

	int m_size{256};
	Frequency m_freq_from{0.0};
	Frequency m_freq_to{1.0};
	Frequency m_freq_cursor{0.0};
	Time m_t_from{0.0};
	Time m_t_to{1.0};
	Time m_t_cursor{0.0};
	float m_amp_cursor{0.0};
	Window::Type m_window_type{Window::Type::Hanning};
	float m_window_beta{5.0f};

	std::vector<Sample> m_in;
	std::vector<Sample> m_out_graph;
	Window m_window;
	Fft m_fft{};
	Sample m_peak{};
};

