
#pragma once

#include <stdint.h>
#include <generator>

#include "config.hpp"

class ChannelMap {

public:
	
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);

	void set_channel_count(int count);
	void draw();
	
	std::generator<int> enabled_channels();
	bool is_channel_enabled(int channel);
	
	static SDL_Color ch_color(int channel);
	static void ch_set_color(int channel, SDL_Color color);


private:
	size_t m_channel_count{0};
	int m_map{0xffff};
};


