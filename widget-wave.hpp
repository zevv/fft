#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "widget.hpp"


class Waveform : public Widget {

public:
	Waveform();
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	void copy_to(Waveform &w);
	void draw(Flap &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

private:

	bool m_agc{true};
	Sample m_peak{};
};
