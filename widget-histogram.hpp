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
	WidgetHistogram(Widget::Info &info);
	~WidgetHistogram() override;

private:
	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;
	void do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r) override;

	bool m_agc{true};
	int m_nbins{64};
	Sample m_vmin{};
	Sample m_vmax{};
	std::array<Histogram, 8> m_hists{};
};
