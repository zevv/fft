#pragma once

#include <SDL3/SDL.h>

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <thread>
#include <atomic>
#include <vector>

#include "rb.hpp"
#include "types.hpp"
#include "wavecache.hpp"
#include "stream-reader.hpp"

class StreamReader;

class StreamCapture {
public:
	StreamCapture(Streams &streams, Rb &rb, Wavecache &wavecache);
	~StreamCapture();

	void enable(bool onoff);
	void add_reader(StreamReader *reader);
	void add_reader(const char *desc);
	size_t channel_count();

private:
	std::vector<StreamReader *> m_readers{};
	std::vector<Sample> m_buf{};
	std::thread m_thread;
	std::atomic<bool> m_enabled{false};
	std::atomic<bool> m_running{false};
	void capture_thread();
	Streams &m_streams;
	Rb &m_rb;
	Wavecache &m_wavecache;
};


