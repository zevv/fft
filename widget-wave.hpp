#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "widget.hpp"


class WidgetWaveform : public Widget {

public:
	WidgetWaveform(Widget::Info &info);
	~WidgetWaveform() override;

private:
	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	bool m_agc{true};
	Sample m_peak{};
};

