#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "fft.hpp"
#include "widget.hpp"


class WidgetSpectrum : public Widget {
public:
	WidgetSpectrum();
	~WidgetSpectrum() override;
	void load(ConfigReader::Node *node) override;
	void save(ConfigWriter &cfg) override;

private:
	Widget *do_copy() override;
	void do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r) override;

	float m_amp_cursor{0.0};
	Fft m_fft{};

	std::vector<Sample> m_in;
	std::vector<Sample> m_out_graph;
};
