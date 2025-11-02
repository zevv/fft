#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "widget.hpp"


class WidgetXY : public Widget {

public:
	WidgetXY();
	~WidgetXY() override;

private:

	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	Widget *do_copy() override;
	void do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r) override;

	SDL_Texture *m_tex{};
};
