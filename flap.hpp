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
#include "widget-waterfall.hpp"


class Flap {
	

public:
	Flap(Widget::Type type);
	~Flap();

	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Flap *copy();

	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	bool has_focus() const { return m_has_focus; }

public:

	Sample graph(SDL_Renderer *rend, SDL_Rect &r,
					Sample *data, size_t data_count, size_t stride,
					float idx_from, float idx_to,
					Sample y_min, Sample y_max);

private:
	Widget::Type m_type{Widget::Type::None};
	Waveform m_waveform;
	Spectrum m_spectrum;
	Waterfall m_waterfall;

	bool m_has_focus;
};
