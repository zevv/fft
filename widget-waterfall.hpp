#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "fft.hpp"
#include "widget.hpp"

class Widgets;

class WidgetWaterfall : public Widget {
public:
	WidgetWaterfall(Widget::Info &info);
	~WidgetWaterfall() override;

private:
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	Fft m_fft{};
	int8_t m_db_min{};
	int8_t m_db_max{};
	bool m_agc{true};
	bool m_fft_approximate{true};
};
