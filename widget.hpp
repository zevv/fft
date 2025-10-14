#pragma once

#include "stream.hpp"


class Widget {
	

public:
	enum Type : int {
		None, Spectogram, Waterfall, Waveform
	};

	Widget(Type type = None);

	void draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_waveform(Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

private:
	Type m_type;
	bool m_channel_map[8];

	struct {
		int count;
		bool agc;
		float peak;
	} m_waveform;

	struct {

	} m_spectrum;
};

