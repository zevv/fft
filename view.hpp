
#pragma once

#include <algorithm>

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "window.hpp"

class View {

public:

	enum class Axis {
		None, Time, Frequency, Amplitude,
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
		n->read("lock", lock);
		if(auto *nc = n->find("time")) {
			nc->read("cursor", time.cursor);
			nc->read("playpos", time.playpos);
			nc->read("from", time.from);
			nc->read("to", time.to);
			nc->read("sel_from", time.sel_from);
			nc->read("sel_to", time.sel_to);
		}
		if(auto *nc = n->find("frequency")) {
			nc->read("cursor", freq.cursor);
			nc->read("from", freq.from);
			nc->read("to", freq.to);
		}
		if(auto *nc = n->find("amplitude")) {
			nc->read("min", amplitude.min);
			nc->read("max", amplitude.max);
		}
		if(auto *nc = n->find("window")) {
			nc->read("size", window.size);
			if(const char *tmp = nc->read_str("type")) {
				window.window_type = Window::str_to_type(tmp);
			}
			nc->read("beta", window.window_beta);
		}
	}

	void save(ConfigWriter &cfg)
	{
		cfg.write("lock", lock);
		cfg.push("time");
		cfg.write("cursor", time.cursor);
		cfg.write("playpos", time.playpos);
		cfg.write("from", time.from);
		cfg.write("to", time.to);
		cfg.write("sel_from", time.sel_from);
		cfg.write("sel_to", time.sel_to);
		cfg.pop();
		cfg.push("frequency");
		cfg.write("from", freq.from);
		cfg.write("to", freq.to);
		cfg.write("cursor", freq.cursor);
		cfg.pop();
		cfg.push("amplitude");
		cfg.write("min", amplitude.min);
		cfg.write("max", amplitude.max);
		cfg.pop();
		cfg.push("window");
		cfg.write("size", window.size);
		cfg.write("type", Window::type_to_str(window.window_type));
		cfg.write("beta", window.window_beta);
		cfg.pop();
	}

	void set_cursor(Config &cfg, SDL_Rect &r, ImVec2 pos)
	{
		if(cfg.x == Axis::Time) {
			time.cursor = time.from + (time.to - time.from) * (pos.x - r.x) / r.w;
		} 
		if(cfg.y == Axis::Time) {
			time.cursor = time.from + (time.to - time.from) * (pos.y - r.y) / r.h;
		}

		if(cfg.x == Axis::Frequency) {
			freq.cursor = freq.from + (freq.to - freq.from) * (pos.x - r.x) / r.w;
		} 
		if(cfg.y == Axis::Frequency) {
			freq.cursor = freq.from + (freq.to - freq.from) * (1.0 - (pos.y - r.y) / r.h);
		}
	}

	void move_cursor(Config &cfg, SDL_Rect &r, ImVec2 delta)
	{
		if(cfg.x == Axis::Time) {
			time.cursor += delta.x / r.w * (time.to - time.from) * 0.1;
		}
		if(cfg.y == Axis::Time) {
			time.cursor += delta.y / r.h * (time.to - time.from) * 0.1;

		}
		if(cfg.x == Axis::Frequency) {
			freq.cursor += delta.x / r.w * (freq.to - freq.from) * 0.1;
		} 
		if(cfg.y == Axis::Frequency) {
			freq.cursor -= delta.y / r.h * (freq.to - freq.from) * 0.1;
		}
	}
	
	Time to_t(Config &cfg, SDL_Rect &r, ImVec2 pos)
	{
		if(cfg.x == Axis::Time) {
			return time.from + (time.to - time.from) * (pos.x - r.x) / r.w;
		} 
		if(cfg.y == Axis::Time) {
			return time.from + (time.to - time.from) * (pos.y - r.y) / r.h;
		}
		return 0.0;
	}


	float from_freq(Config &cfg, SDL_Rect &r, Frequency f) {
		if(cfg.x == Axis::Frequency) {
			return r.x + r.w * (f- freq.from) / (freq.to - freq.from);
		}
		if(cfg.y == Axis::Frequency) {
			return r.y + r.h * (1.0 - (f - freq.from) / (freq.to - freq.from));
		}
		return 0.0;
	}
	
	float from_t(Config &cfg, SDL_Rect &r, Time t) {
		if(cfg.x == Axis::Time) {
			return r.x + r.w * (t - time.from) / (time.to - time.from);
		} 
		if(cfg.y == Axis::Time) {
			return r.y + r.h * (t - time.from) / (time.to - time.from);
		}
		return 0.0;
	}

	void pan(Config &cfg, SDL_Rect &r, ImVec2 delta) {
		if(cfg.x == Axis::Time) {
			double delta_t = delta.x/r.w;
			pan_t(delta_t);
		} 
		if(cfg.y == Axis::Time) {
			double delta_t = delta.y/r.h;
			pan_t(delta_t);
		}
		if(cfg.x == Axis::Frequency) {
			double delta_f = -delta.x/r.w;
			pan_freq(delta_f);
		} 
		if(cfg.y == Axis::Frequency) {
			double delta_f = delta.y/r.h;
			pan_freq(delta_f);
		}
	}

	void zoom(Config &cfg, SDL_Rect &r, ImVec2 delta) {
		if(cfg.x == Axis::Time) {
			zoom_t(delta.x, ZoomAnchor::Cursor);
		} 
		if(cfg.y == Axis::Time) {
			zoom_t(delta.y, ZoomAnchor::Cursor);
		}
		if(cfg.x == Axis::Frequency) {
			zoom_freq(delta.x);
		} 
		if(cfg.y == Axis::Frequency) {
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
