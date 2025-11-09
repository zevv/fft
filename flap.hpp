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


class Widgets {
	

public:
	Widgets(Widget::Type type);
	~Widgets();

	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Widgets *copy();

	void draw(View &view, Stream &stream, SDL_Renderer *rend, SDL_Rect &r);

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
};
