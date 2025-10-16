
#pragma once

#include <algorithm>

#include "window.hpp"

class View {

public:
	View()
		: from(0)
		, to(1024)
		, m_width_min(100)
		, m_width_max(1024 * 1024)
	{}

	void clamp() {
		float a1 = 0.9;
		float a2 = 0.1;
		float span = to - from;
		if(span < m_width_min) {
			from = from * a1 + (to - m_width_min) * a2;
			to = to * a1 + (from + m_width_min) * a2;
		}
	};

	void pan(float delta) {
		from += delta;
		to += delta;
	};

	void zoom(float f)
	{
		from += (cursor - from) * f;
		to   -= (to - cursor) * f;
	};

	float x_to_idx(float x, SDL_Rect &r)
	{
		return to + (from - to) * (x - r.x) / r.w;
	}

	float idx_to_x(float idx, SDL_Rect &r)
	{
		return r.x + r.w * (idx - to) / (from - to);
	}

	float didx_to_dx(float didx, SDL_Rect &r)
	{
		return r.w * didx / (from - to);
	}

	float dx_to_didx(float dx, SDL_Rect &r)
	{
		return dx * (to - from) / r.w;
	}
	
	float from;
	float to;
	float cursor;
	int fft_width;
	Window *window;

private:

	float m_width_min;
	float m_width_max;
};

