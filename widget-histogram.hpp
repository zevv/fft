#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "widget.hpp"
#include "histogram.hpp"


class WidgetHistogram : public Widget {

public:
	WidgetHistogram();
	~WidgetHistogram() override;
	void load(ConfigReader::Node *node) override;
	void save(ConfigWriter &cfg) override;

private:
	Widget *do_copy() override;
	void do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r) override;

	bool m_agc{true};
	int m_nbins{64};
	Sample m_vmin{};
	Sample m_vmax{};
	std::array<Histogram, 8> m_hists{};
};
