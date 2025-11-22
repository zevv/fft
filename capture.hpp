#pragma once

#include <SDL3/SDL.h>

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <thread>
#include <atomic>
#include <vector>

#include "types.hpp"
#include "source.hpp"

class Source;

class Capture {
public:
	Capture(Stream &stream);
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
	SDL_AudioSpec m_spec{};
	size_t m_frames_event{};
};


