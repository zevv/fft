
#pragma once

#include <stdint.h>

#include "config.hpp"

class ChannelMap {

public:
	
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);

	void set_channel_count(int count);
	bool ch_enabled(int channel);
	void ch_set(int channel, bool enabled);
	void ch_toggle(int channel);
	void draw();
	
	static SDL_Color ch_color(int channel);

private:
	size_t m_channel_count{0};
	int m_map{0xffff};
};


