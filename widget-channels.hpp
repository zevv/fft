#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "widget.hpp"


class WidgetChannels : public Widget {

public:
	WidgetChannels(Widget::Info &info);
	~WidgetChannels() override;

private:

	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;
};
