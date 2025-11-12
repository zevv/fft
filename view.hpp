
#pragma once

#include <algorithm>

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "window.hpp"

class View {

public:
	View()
	{
	}

	void load(ConfigReader::Node *n)
	{
		n->read("t_cursor", time.cursor);
		n->read("t_playpos", time.playpos);
		n->read("t_from", time.from);
		n->read("t_to", time.to);
		n->read("t_sel_from", time.sel_from);
		n->read("t_sel_to", time.sel_to);
		n->read("freq_from", freq.from);
		n->read("freq_to", freq.to);
		n->read("freq_cursor", freq.cursor);
		n->read("window_size", window.size);
		if(const char *tmp = n->read_str("window_type")) {
			window.window_type = Window::str_to_type(tmp);
		}
		n->read("window_beta", window.window_beta);
		n->read("lock", lock);
	}

	void save(ConfigWriter &cfg)
	{
		cfg.write("t_cursor", time.cursor);
		cfg.write("t_playpos", time.playpos);
		cfg.write("t_from", time.from);
		cfg.write("t_to", time.to);
		cfg.write("t_sel_from", time.sel_from);
		cfg.write("t_sel_to", time.sel_to);
		cfg.write("freq_from", freq.from);
		cfg.write("freq_to", freq.to);
		cfg.write("freq_cursor", freq.cursor);
		cfg.write("window_size", window.size);
		cfg.write("window_type", Window::type_to_str(window.window_type));
		cfg.write("window_beta", window.window_beta);
		cfg.write("lock", lock);
	}

	Time x_to_t(float x, SDL_Rect &r) {
		return time.from + (time.to - time.from) * (x - r.x) / r.w;
	}

	float t_to_x(Time t, SDL_Rect &r) {
		return r.x + r.w * (t - time.from) / (time.to - time.from);
	}

	Time dx_to_dt(float dx, SDL_Rect &r) {
		return dx * (time.to - time.from) / r.w;
	}

	Frequency x_to_freq(float x, SDL_Rect &r) {
		return freq.from + (freq.to - freq.from) * (x - r.x) / r.w;
	}

	float freq_to_x(Frequency f, SDL_Rect &r) {
		return r.x + r.w * (f- freq.from) / (freq.to - freq.from);
	}
	
	Frequency dx_to_dfreq(float dx, SDL_Rect &r) {
		return dx * (freq.to - freq.from) / r.w;
	}

	Time y_to_t(float y, SDL_Rect &r) {
		return time.from + (time.to - time.from) * (y - r.y) / r.h;
	}

	float t_to_y(Time t, SDL_Rect &r) {
		return r.y + r.h * (t - time.from) / (time.to - time.from);
	}

	Time dy_to_dt(float dy, SDL_Rect &r) {
		return dy * (time.to - time.from) / r.h;
	}

	void pan_t(float f) {
		Time dt = -f * (time.to - time.from);
		time.from += dt;
		time.to += dt;
		clamp();
	};

	enum ZoomAnchor {
		Cursor,
		Middle,
	};

	void zoom_t(float f, ZoomAnchor za=Cursor) {
		f /= 100.0;
		Time t_mid = (za == Cursor) ? time.cursor : 0.5 * (time.from + time.to);
		time.from += (t_mid - time.from) * f;
		time.to   -= (time.to - t_mid) * f;
		clamp();
	};

	void pan_freq(float f) {
		Frequency df = f * (freq.to - freq.from);
		freq.from += df;
		freq.to += df;
		clamp();
	};

	void zoom_freq(float f) {
		f /= 100.0;
		freq.from += (freq.cursor - freq.from) * f;
		freq.to   -= (freq.to - freq.cursor) * f;
		clamp();
	};

	void clamp() {
		if(time.from >= time.to) {
			Time mid = 0.5 * (time.from + time.to);
			time.from = mid - 0.001;
			time.to   = mid + 0.001;
		}
		if(freq.from < 0.0) freq.from = 0.0;
		if(freq.to   > 1.0) freq.to   = 1.0;
		if(freq.from >= freq.to) {
			Frequency mid = 0.5 * (freq.from + freq.to);
			freq.from = mid - 0.001;
			freq.to   = mid + 0.001;
		}
		window.size = std::clamp(window.size, 16, 16384);
	}


	struct VTime {
		Time from{0.0};
		Time to{1.0};
		Time cursor{0.5};
		Time playpos{0.0};
		Time analysis{0.0};
		Time sel_from{0.0};
		Time sel_to{0.0};
	};

	struct VFreq {
		Frequency from{0.0};
		Frequency to{22000.0};
		Frequency cursor{11000.0};
	};

	struct VWindow {
		int size{256};
		Window::Type window_type{Window::Type::Hanning};
		double window_beta{0.5};
	};

	struct VAperture {
		float center{-40.0f};
		float range{80.0f};
	};
	
	bool lock{true};
	VTime time{};
	VFreq freq{};
	VWindow window{};
	VAperture aperture{};
};
