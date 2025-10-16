
#pragma once

#include <algorithm>

#include "window.hpp"

class View {

public:
	View()
		: wave_from(0)
		, wave_to(1024)
		, m_width_min(100)
		, m_width_max(1024 * 1024)
	{}

	void clamp() {
		float a1 = 0.9;
		float a2 = 0.1;
		float span = wave_to - wave_from;
		if(span < m_width_min) {
			wave_from = wave_from * a1 + (wave_to - m_width_min) * a2;
			wave_to = wave_to * a1 + (wave_from + m_width_min) * a2;
		}
	};

	void pan(float delta) {
		wave_from += delta;
		wave_to += delta;
	};

	void zoom(float f)
	{
		wave_from += (cursor - wave_from) * f;
		wave_to   -= (wave_to - cursor) * f;
	};

	float x_to_idx(float x, SDL_Rect &r)
	{
		return wave_to + (wave_from - wave_to) * (x - r.x) / r.w;
	}

	float idx_to_x(float idx, SDL_Rect &r)
	{
		return r.x + r.w * (idx - wave_to) / (wave_from - wave_to);
	}

	float didx_to_dx(float didx, SDL_Rect &r)
	{
		return r.w * didx / (wave_from - wave_to);
	}

	float dx_to_didx(float dx, SDL_Rect &r)
	{
		return dx * (wave_to - wave_from) / r.w;
	}
	
	float wave_from;
	float wave_to;
	float cursor;
	int fft_width;
	Window *window;

private:

	float m_width_min;
	float m_width_max;
};
