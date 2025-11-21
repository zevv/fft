
#include <imgui.h>
#include <SDL3/SDL.h>
#include <assert.h>

#include "style.hpp"

struct ColorDef {
	const char *name;
	Style::Color color;

};

#define DEF_COLOR(id, r, g, b, a) \
	[Style::ColorId::id] = { #id, Style::Color{ r, g, b, a } }

static ColorDef colordef_list[Style::COUNT] = {
	DEF_COLOR(Background,        0.00, 0.00, 0.00, 1.00),
	DEF_COLOR(Cursor,            1.00, 1.00, 0.75, 1.00),
	DEF_COLOR(Playpos,           0.00, 0.75, 1.00, 1.00),
	DEF_COLOR(HarmonicHelper,    0.50, 0.50, 0.50, 1.00),
	DEF_COLOR(Grid1,             0.25, 0.25, 0.25, 1.00),
	DEF_COLOR(AnalysisWindow,    0.00, 0.50, 0.00, 0.50),
	DEF_COLOR(PanelBorder,       0.00, 0.50, 0.50, 1.00),
	DEF_COLOR(ToggleButtonOff,   0.26, 0.26, 0.38, 1.00),
	DEF_COLOR(ToggleButtonOn,    0.26, 0.59, 0.98, 1.00),

	// https://medialab.github.io/iwanthue/
	DEF_COLOR(Channel1 + 0,      0.00, 0.60, 0.80, 1.00),
	DEF_COLOR(Channel1 + 1,      1.00, 0.40, 0.20, 1.00),
	DEF_COLOR(Channel1 + 2,      0.16, 0.65, 0.00, 1.00),
	DEF_COLOR(Channel1 + 3,      0.84, 0.35, 1.00, 1.00),
	DEF_COLOR(Channel1 + 4,      0.50, 0.55, 0.00, 1.00),
	DEF_COLOR(Channel1 + 5,      0.50, 0.45, 1.00, 1.00),
	DEF_COLOR(Channel1 + 6,      0.95, 0.25, 0.68, 1.00),
	DEF_COLOR(Channel1 + 7,      0.94, 0.62, 0.09, 1.00),
	DEF_COLOR(ChannelDisabled,   0.20, 0.20, 0.20, 1.00),
};


Style::Color Style::color(int cid)
{
	assert(cid >= 0 && cid < Style::COUNT);
	return colordef_list[cid].color;
}


void Style::set_color(int cid, const Color &color)
{
	assert(cid >= 0 && cid < Style::COUNT);
	colordef_list[cid].color = color;
}


Style::Color Style::channel_color(int ch)
{
	return color(Style::ColorId::Channel1 + (ch % 8));
}


void Style::set_channel_color(int ch, const ImVec4 &color)
{
	Color c = { color.x, color.y, color.z, color.w };
	set_color(Style::ColorId::Channel1 + (ch % 8), c);
}


int SDL_SetRenderDrawColor(SDL_Renderer *rend, Style::Color col)
{
	SDL_Color c = col;
	return SDL_SetRenderDrawColor(rend, c.r, c.g, c.b, c.a);
}	
