#pragma once

#include <SDL3/SDL.h>

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <thread>
#include <atomic>

#include "rb.hpp"
#include "types.hpp"
#include "wavecache.hpp"



class Streams;

class StreamPlayer {
public:
	StreamPlayer(Streams &streams);
	void enable(bool onoff);
	void seek(Time tpos);
	void audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount);

private:
	Streams &m_streams;	
	std::atomic<Time> m_play_pos{0};
	SDL_AudioStream *m_sdl_audio_stream{nullptr};
	size_t m_idx{};
	size_t m_idx_prev{};
	double m_xfade{};
	uint32_t m_t_event{};
};


class StreamReader;

class Streams {
public:

	Streams();
	~Streams();
	size_t channel_count() { return m_channel_count; }
	void allocate(size_t depth);
	size_t frames_avail();
	Sample *peek(size_t *stride, size_t *used = nullptr);
	Wavecache::Range *peek_wavecache(size_t *stride, size_t *used = nullptr);
	void add_reader(StreamReader *reader);
	void capture_enable(bool onoff);
	
	class StreamPlayer player;

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




