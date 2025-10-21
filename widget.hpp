#pragma once

#include <SDL3/SDL.h>
#include <fftw3.h>
#include <imgui.h>

#include "stream.hpp"
#include "view.hpp"
#include "config.hpp"
#include "types.hpp"
#include "widget-wave.hpp"
#include "widget-spectrum.hpp"


class Widget {
	

public:
	enum Type : int {
		None, Waveform, Spectrum, Waterfall
	};

	Widget(Type type = None);
	~Widget();

	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Widget *copy();


	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_spectrum(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	bool has_focus() const { return m_has_focus; }
	bool channel_enabled(int channel) const { return m_channel_map[channel]; }
	ImVec4 channel_color(int channel);

	static const char* type_to_string(Type type);
	static Type string_to_type(const char *str);
	static const char **type_names();
	static size_t type_count();

private:

	friend class Waveform;
	friend class Spectrum;

	Sample graph(SDL_Renderer *rend, SDL_Rect &r, 
					 ImVec4 &col, Sample *data, size_t stride,
					 float idx_from, float idx_to,
					 int idx_min, int idx_max,
					 Sample y_min, Sample y_max);
	

	Type m_type;
	bool m_channel_map[8];
	class Waveform m_waveform;
	class Spectrum m_spectrum;

	bool m_has_focus;
};
