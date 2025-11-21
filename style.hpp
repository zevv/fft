
#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>

#include "config.hpp"

class Style {

public:

	struct Color {
		float r, g, b, a;
		operator ImVec4() const { return ImVec4(r, g, b, a); }
		operator SDL_FColor() const { return SDL_FColor{ r, g, b, a }; }
		operator SDL_Color() const { return SDL_Color{ uint8_t(r * 255), uint8_t(g * 255), uint8_t(b * 255), uint8_t(a * 255) }; }
	};

	enum ColorId {
		Background,
		Cursor,
		Playpos,
		HarmonicHelper,
		Grid1,
		AnalysisWindow,
		PanelBorder,
		ToggleButtonOff,
		ToggleButtonOn,
		Channel1,
		Channel8 = Channel1 + 7,
		ChannelDisabled,
		COUNT,
	};

	static void load(ConfigReader::Node *node);
	static void save(ConfigWriter &cfg);

	static Color color(int cid);
	static Color channel_color(int ch);
	static void set_color(int cid, const Color &color);
	static void set_channel_color(int ch, const ImVec4 &color);

};

		
int SDL_SetRenderDrawColor(SDL_Renderer *rend, Style::Color col);

