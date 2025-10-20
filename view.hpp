
#pragma once

#include <algorithm>

#include "window.hpp"
#include "config.hpp"

class View {

public:
	View()
	{
		reset();
	}

	void reset()
	{
		wave_from = -1000;
		wave_to = 1000;
		cursor = 512;
	}

	void load(ConfigReader::Node *n)
	{
		n->read("cursor", cursor);
		n->read("wave_from", wave_from);
		n->read("wave_to", wave_to);
	}

	void save(ConfigWriter &cfg)
	{
		cfg.write("cursor", cursor);
		cfg.write("wave_from", wave_from);
		cfg.write("wave_to", wave_to);
	}

	float wave_from;
	float wave_to;
	float cursor;
	float srate;
	Window *window;

};
