
#pragma once

#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

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

	void load(ConfigReader::Node *n);
	void save(ConfigWriter &cfg);

	void set_cursor(Config &cfg, SDL_Rect &r, ImVec2 pos);
	void move_cursor(Config &cfg, SDL_Rect &r, ImVec2 delta);
	
	float from_t(Config &cfg, SDL_Rect &r, Time t);
	float from_freq(Config &cfg, SDL_Rect &r, Frequency f);
	
	void pan(Config &cfg, SDL_Rect &r, ImVec2 delta);
	void zoom(Config &cfg, SDL_Rect &r, ImVec2 delta);

	void pan_t(float f);
	void zoom_t(float f);
	void pan_freq(float f);
	void clamp();

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
		double from{-1.0f}; // amplitude view min
		double to{+1.0f}; // amplitude view max
		double cursor{0.0f}; // user amplitude cursor position
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
