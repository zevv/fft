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

private:
	Widget *do_copy() override;
	void do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r) override;

	Fft m_fft{};
	float m_db_min{};
	float m_db_max{};
	bool m_agc{true};
};
