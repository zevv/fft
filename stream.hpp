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
#include "stream-reader.hpp"
#include "stream-player.hpp"


class StreamReader;

class Streams {
public:

	Streams();
	~Streams();
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);

	size_t channel_count() { return m_channel_count; }
	void allocate(size_t depth);
	size_t frames_avail();
	Sample *peek(size_t *stride, size_t *used = nullptr);
	Wavecache::Range *peek_wavecache(size_t *stride, size_t *used = nullptr);
	void add_reader(StreamReader *reader);
	void capture_enable(bool onoff);
	
	StreamPlayer player;

private:

	size_t m_depth{};
	size_t m_channel_count{};
	size_t m_frame_size{};
	Rb m_rb;
	Wavecache m_wavecache;
	std::vector<StreamReader *> m_readers{};

	struct {
		std::thread thread;
		std::atomic<bool> enabled{false};
		std::atomic<bool> running{false};
	} m_capture{};

	void capture_thread();

public:
};




