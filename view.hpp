
#pragma once

#include <algorithm>

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "window.hpp"

class View {

public:

	enum class Axis {
		None, Time, Frequency, Amplitude
	};

	struct Config {
		Axis x{Axis::None};
		Axis y{Axis::None};
	};

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
		n->read("amplitude_min", amplitude.min);
		n->read("amplitude_max", amplitude.max);
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
		cfg.write("amplitude_min", amplitude.min);
		cfg.write("amplitude_max", amplitude.max);
		cfg.write("window_size", window.size);
		cfg.write("window_type", Window::type_to_str(window.window_type));
		cfg.write("window_beta", window.window_beta);
		cfg.write("lock", lock);
	}

	void set_cursor(Config &cfg, SDL_Rect &r, ImVec2 pos)
	{
		if(cfg.x == Axis::Time) {
			time.cursor = time.from + (time.to - time.from) * (pos.x - r.x) / r.w;
		} else if(cfg.y == Axis::Time) {
			time.cursor = time.from + (time.to - time.from) * (pos.y - r.y) / r.h;
		}

		if(cfg.x == Axis::Frequency) {
			freq.cursor = freq.from + (freq.to - freq.from) * (pos.x - r.x) / r.w;
		} else if(cfg.y == Axis::Frequency) {
			freq.cursor = freq.from + (freq.to - freq.from) * (1.0 - (pos.y - r.y) / r.h);
		}
	}

	void move_cursor(Config &cfg, SDL_Rect &r, ImVec2 delta)
	{
		if(cfg.x == Axis::Time) {
			time.cursor += delta.x / r.w * (time.to - time.from) * 0.1;
		} else if(cfg.y == Axis::Time) {
			time.cursor += delta.y / r.h * (time.to - time.from) * 0.1;

		}
		if(cfg.x == Axis::Frequency) {
			freq.cursor += delta.x / r.w * (freq.to - freq.from) * 0.1;
		} else if(cfg.y == Axis::Frequency) {
			freq.cursor -= delta.y / r.h * (freq.to - freq.from) * 0.1;
		}
	}
	
	Time to_t(Config &cfg, SDL_Rect &r, ImVec2 pos)
	{
		if(cfg.x == Axis::Time) {
			return time.from + (time.to - time.from) * (pos.x - r.x) / r.w;
		} else {
			return time.from + (time.to - time.from) * (pos.y - r.y) / r.h;
		}
	}


	float from_freq(Config &cfg, SDL_Rect &r, Frequency f) {
		if(cfg.x == Axis::Frequency) {
			return r.x + r.w * (f- freq.from) / (freq.to - freq.from);
		} else {
			return r.y + r.h * (1.0 - (f - freq.from) / (freq.to - freq.from));
		}
	}
	
	float from_t(Config &cfg, SDL_Rect &r, Time t) {
		if(cfg.x == Axis::Time) {
			return r.x + r.w * (t - time.from) / (time.to - time.from);
		} else {
			return r.y + r.h * (t - time.from) / (time.to - time.from);
		}
	}

	void pan(Config &cfg, SDL_Rect &r, ImVec2 delta) {
		if(cfg.x == Axis::Time) {
			double delta_t = delta.x/r.w;
			pan_t(delta_t);
		} else if(cfg.y == Axis::Time) {
			double delta_t = delta.y/r.h;
			pan_t(delta_t);
		}
		if(cfg.x == Axis::Frequency) {
			double delta_f = -delta.x/r.w;
			pan_freq(delta_f);
		} else if(cfg.y == Axis::Frequency) {
			double delta_f = delta.y/r.h;
			pan_freq(delta_f);
		}
	}

	void zoom(Config &cfg, SDL_Rect &r, ImVec2 delta) {
		if(cfg.x == Axis::Time) {
			zoom_t(delta.x, ZoomAnchor::Cursor);
		} else if(cfg.y == Axis::Time) {
			zoom_t(delta.y, ZoomAnchor::Cursor);
		}
		if(cfg.x == Axis::Frequency) {
			zoom_freq(delta.x);
		} else if(cfg.y == Axis::Frequency) {
			zoom_freq(delta.y);
		}

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
		//if(freq.from < 0.0) freq.from = 0.0;
		//if(freq.to   > 1.0) freq.to   = 1.0;
		if(freq.from >= freq.to) {
			Frequency mid = 0.5 * (freq.from + freq.to);
			freq.from = mid - 0.001;
			freq.to   = mid + 0.001;
		}
		window.size = std::clamp(window.size, 16, 32768);
	}


	struct VTime {
		Time from{0.0}; // time view range start
		Time to{1.0}; // time view range end
		Time cursor{0.5}; // user time cursor position
		Time playpos{0.0}; // current playback position
		Time analysis{0.0}; // current analysis position, synced with capture/playback state
		Time sel_from{0.0}; // selection range start
		Time sel_to{0.0}; // selection range end
	};

	struct VFreq {
		Frequency from{0.0}; // frequency view range start
		Frequency to{22000.0}; // frequency view range end
		Frequency cursor{11000.0}; // user frequency cursor position
	};

	struct VWindow {
		int size{256}; // analysis window size
		Window::Type window_type{Window::Type::Hanning}; // analysis window type
		double window_beta{0.5}; // analysis window beta (gaussian, kaiser)
	};

	struct VAmplitude {
		float min{-1.0f}; // amplitude view min
		float max{+1.0f}; // amplitude view max
	};

	struct VAperture {
		float center{-40.0f}; // waterfall aperture dB center
		float range{80.0f}; // waterfall aperture dB range
	};
	
	bool lock{true}; // view is locked to global view
	VTime time{};
	VFreq freq{};
	VWindow window{};
	VAmplitude amplitude{};
	VAperture aperture{};
};
