#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"

class Widget;

class Waveform {

public:
	Waveform();
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	void copy_to(Waveform &w);
	void draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

private:

	Time x_to_t(float x, SDL_Rect &r) {
		return m_t_from - (m_t_from - m_t_to) * (x - r.x) / r.w;
	}

	float t_to_x(Time t, SDL_Rect &r) {
		return r.x + r.w * (m_t_from - t) / (m_t_from - m_t_to);
	}

	Time dx_to_dt(float dx, SDL_Rect &r) {
		return dx * (m_t_to - m_t_from) / r.w;
	}

	void pan(float delta) {
		m_t_from += delta;
		m_t_to += delta;
	};

	void zoom(float f) {
		m_t_from += (m_t_cursor - m_t_from) * f;
		m_t_to   -= (m_t_to - m_t_cursor) * f;
	};

	bool m_agc{true};
	Sample m_peak{};
	Time m_t_from{};
	Time m_t_to{};
	Time m_t_cursor{};
};
