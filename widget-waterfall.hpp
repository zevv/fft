#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "fft.hpp"
#include "widget.hpp"

class Flap;

class WidgetWaterfall : public Widget {
public:
	WidgetWaterfall();
	~WidgetWaterfall() override;
	void load(ConfigReader::Node *node) override;
	void save(ConfigWriter &cfg) override;

private:
	Widget *do_copy() override;
	void do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r) override;

	std::vector<Sample> m_in;
	std::vector<Sample> m_out_graph;
	Fft m_fft{};
	float m_peak{};
};
