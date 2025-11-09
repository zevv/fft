#pragma once

#include <SDL3/SDL.h>

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <thread>
#include <atomic>

#include "rb.hpp"
#include "config.hpp"
#include "types.hpp"
#include "wavecache.hpp"
#include "stream-player.hpp"
#include "stream-capture.hpp"


class Source;

class Streams {
public:

	Streams();
	~Streams();
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);

	void set_sample_rate(Samplerate srate);
	size_t channel_count() { return m_channel_count; }
	void allocate(size_t depth);
	Samplerate sample_rate() { return m_srate; }
	Sample *peek(size_t *stride, size_t *used = nullptr);
	Wavecache::Range *peek_wavecache(size_t *stride, size_t *used = nullptr);
	
	StreamPlayer player;
	StreamCapture capture;

private:

	size_t m_depth{};
	size_t m_channel_count{};
	size_t m_frame_size{};
	Rb m_rb;
	Wavecache m_wavecache;
	Samplerate m_srate{};


public:
};
