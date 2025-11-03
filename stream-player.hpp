
#pragma once

#include <atomic>
#include <algorithm>

#include <SDL3/SDL.h>

#include "config.hpp"

class Streams;

class StreamPlayer {
public:
	
	struct Channel {
		bool enabled{true};
		float volume{0.3f};
		float pan{0.0f};
	};

	StreamPlayer(Streams &streams);
	~StreamPlayer();
	
	void load(ConfigReader::Node *n);
	void save(ConfigWriter &cw);

	void set_channel_count(size_t count);
	void enable(bool onoff);
	void seek(Time tpos);
	void audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount);

	float get_speed() const { return m_speed; }
	void set_speed(float speed) { m_speed = std::clamp(speed, 0.05f, 20.0f); }

	std::vector<Channel>& get_channels() { return m_channels; }

private:
	Streams &m_streams;	
	Samplerate m_srate;
	std::vector<Channel> m_channels;
	std::atomic<Time> m_play_pos{0};
	SDL_AudioStream *m_sdl_audio_stream{nullptr};
	size_t m_idx{};
	size_t m_idx_prev{};
	double m_xfade{};
	uint32_t m_t_event{};
	size_t m_frames_event{};
	size_t m_frame_size;
	size_t m_buf_frames;
	std::atomic<float> m_speed{1.0f};
	std::vector<float> m_buf{};
};


