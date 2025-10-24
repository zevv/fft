#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "widget.hpp"


class Waveform : public Widget {

public:
	Waveform();
	~Waveform() override;
	void load(ConfigReader::Node *node) override;
	void save(ConfigWriter &cfg) override;
	void copy_to(Waveform &w);
	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

private:

	bool m_agc{true};
	Sample m_peak{};
};
