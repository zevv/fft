
#pragma once

#include <algorithm>

#include "window.hpp"
#include "config.hpp"

class View {

public:
	View()
		: m_width_min(100)
	{
		reset();
	}

	void reset()
	{
		wave_from = -1000;
		wave_to = 1000;
		cursor = 512;
	}

	void clamp() {
		float a1 = 0.9;
		float a2 = 0.1;
		float span = wave_to - wave_from;
		if(span < m_width_min) {
			wave_from = a1 * wave_from + a2 * (wave_to - m_width_min);
			wave_to   = a1 * wave_to   + a2 * (wave_from + m_width_min);
		}
	};

	float db_to_y(float db, SDL_Rect &r)
	{
		return r.y - r.h * db / 100.0f;
	}
	
	float y_to_db(float y, SDL_Rect &r)
	{
		return 100.0f * (y - r.y) / r.h;
	}


	void load(ConfigReader::Node *n)
	{
		n->read("cursor", cursor);
		n->read("wave_from", wave_from);
		n->read("wave_to", wave_to);
	}

	void save(ConfigWriter &cfg)
	{
		cfg.write("cursor", cursor);
		cfg.write("wave_from", wave_from);
		cfg.write("wave_to", wave_to);
	}

	float wave_from;
	float wave_to;
	float cursor;
	Window *window;

private:

	float m_width_min;
};
