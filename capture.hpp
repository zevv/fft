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
#include "source.hpp"

class Source;

class Capture {
public:
	Capture(Stream &stream, Rb &rb, Wavecache &wavecache);
	~Capture();

	void set_sample_rate(Samplerate srate);
	void start();
	void resume();
	void pause();
	void add_source(const char *desc);
	size_t channel_count();
	const std::vector<Source *> &sources() { return m_sources; }

private:
	std::vector<Source *> m_sources{};
	std::vector<Sample> m_buf{};
	std::thread m_thread;
	std::atomic<bool> m_running{false};
	void capture_thread();
	Stream &m_stream;
	Rb &m_rb; // TODO remove
	Wavecache &m_wavecache;
	SDL_AudioSpec m_spec{};
};


