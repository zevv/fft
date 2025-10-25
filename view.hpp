
#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"

class View {

public:
	View()
	{
	}

	void load(ConfigReader::Node *n)
	{
		n->read("t_cursor", t_cursor);
		n->read("t_from", t_from);
		n->read("t_to", t_to);
		n->read("freq_from", freq_from);
		n->read("freq_to", freq_to);
		n->read("freq_cursor", freq_cursor);
	}

	void save(ConfigWriter &cfg)
	{
		cfg.write("t_cursor", t_cursor);
		cfg.write("t_from", t_from);
		cfg.write("t_to", t_to);
		cfg.write("freq_from", freq_from);
		cfg.write("freq_to", freq_to);
		cfg.write("freq_cursor", freq_cursor);
	}

	Time x_to_t(float x, SDL_Rect &r) {
		return t_from + (t_to - t_from) * (x - r.x) / r.w;
	}

	float t_to_x(Time t, SDL_Rect &r) {
		return r.x + r.w * (t - t_from) / (t_to - t_from);
	}

	Time dx_to_dt(float dx, SDL_Rect &r) {
		return dx * (t_to - t_from) / r.w;
	}

	Frequency x_to_freq(float x, SDL_Rect &r) {
		return freq_from + (freq_to - freq_from) * (x - r.x) / r.w;
	}

	float freq_to_x(Frequency freq, SDL_Rect &r) {
		return r.x + r.w * (freq - freq_from) / (freq_to - freq_from);
	}
	
	Frequency dx_to_dfreq(float dx, SDL_Rect &r) {
		return dx * (freq_to - freq_from) / r.w;
	}

	Time y_to_t(float y, SDL_Rect &r) {
		return t_from + (t_to - t_from) * (y - r.y) / r.h;
	}

	float t_to_y(Time t, SDL_Rect &r) {
		return r.y + r.h * (t - t_from) / (t_to - t_from);
	}

	Time dy_to_dt(float dy, SDL_Rect &r) {
		return dy * (t_to - t_from) / r.h;
	}

	void pan_t(float f) {
		printf("pan t: %f\n", f);
		Time dt = -f * (t_to - t_from);
		t_from += dt;
		t_to += dt;
	};

	void zoom_t(float f) {
		f /= 50.0;
		t_from += (t_cursor - t_from) * f;
		t_to   -= (t_to - t_cursor) * f;
	};

	void pan_freq(float f) {
		Frequency df = f * (freq_to - freq_from);
		freq_from += df;
		freq_to += df;
	};

	void zoom_freq(float f) {
		f /= 50.0;
		freq_from += (freq_cursor - freq_from) * f;
		freq_to   -= (freq_to - freq_cursor) * f;
	};

	float t_from;
	float t_to;
	float t_cursor;
	float freq_from;
	float freq_to;
	float freq_cursor;
	float srate;
};
