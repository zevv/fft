
#pragma once

#include <atomic>
#include <algorithm>

#include <SDL3/SDL.h>

#include "config.hpp"
#include "biquad.hpp"
#include "hilbert.hpp"
#include "freqshift.hpp"

class Stream;

class Player {
public:

	struct Config {
		Db master{1.0};
		Frequency shift{0.0};
		Factor pitch{1.0};
		Factor stretch{1.0};
		Frequency freq_hp{0.0};
		Frequency freq_lp{1.0};
		bool operator==(const Config &other) const = default;
	};
	
	struct ChannelConfig {
		bool enabled{true};
		Db level{0.3f};
		double pan{0.0f};
		bool operator==(const ChannelConfig &other) const = default;
	};

	Player(Stream &stream);
	~Player();
	
	void load(ConfigReader::Node *n);
	void save(ConfigWriter &cw);

	Config config();
	void set_config(Config &cfg);

	ChannelConfig channel_config(size_t ch);
	void set_channel_config(size_t ch, ChannelConfig &channel);

	void set_channel_count(size_t count);
	void set_sample_rate(Samplerate srate);
	void pause();
	void resume();
	void seek(Time tpos);
	void audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount);

private:
	Stream &m_stream;	
	Samplerate m_srate;
	std::atomic<Config> m_config{ Config{} };
	std::vector<ChannelConfig> m_channel_config{ ChannelConfig{} };
	std::atomic<Time> m_play_pos{0};
	SDL_AudioStream *m_sdl_audio_stream{nullptr};
	size_t m_idx{};
	size_t m_idx_prev{};
	size_t m_xfade{};
	uint32_t m_t_event{};
	size_t m_frames_event{};
	size_t m_frame_size;
	size_t m_buf_frames;
	std::vector<float> m_buf{};
	struct {
		Biquad bq_hp[2][2];
		Biquad bq_lp[2][2];
	} m_filter;
	FreqShift m_freqshift[2];
};
