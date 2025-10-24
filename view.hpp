
#pragma once

#include <algorithm>

#include "window.hpp"
#include "config.hpp"

class View {

public:
	View()
	{
	}

	void load(ConfigReader::Node *n)
	{
		n->read("t_cursor", t_cursor);
		n->read("t_from", t_from);
		n->read("t_to", t_to);
		n->read("freq_from", freq_from);
		n->read("freq_to", freq_to);
		n->read("freq_cursor", freq_cursor);
	}

	void save(ConfigWriter &cfg)
	{
		cfg.write("t_cursor", t_cursor);
		cfg.write("t_from", t_from);
		cfg.write("t_to", t_to);
		cfg.write("freq_from", freq_from);
		cfg.write("freq_to", freq_to);
		cfg.write("freq_cursor", freq_cursor);
	}

	float t_from;
	float t_to;
	float t_cursor;
	float freq_from;
	float freq_to;
	float freq_cursor;
	float srate;
};
