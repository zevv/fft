
#pragma once

#include <atomic>
#include <algorithm>

#include <SDL3/SDL.h>

#include "config.hpp"
#include "biquad.hpp"

class Streams;

class StreamPlayer {
public:
	
	struct Channel {
		bool enabled{true};
		float gain{0.3f};
		float pan{0.0f};
	};

	StreamPlayer(Streams &streams);
	~StreamPlayer();
	
	void load(ConfigReader::Node *n);
	void save(ConfigWriter &cw);

	void set_channel_count(size_t count);
	void set_sample_rate(Samplerate srate);
	void enable(bool onoff);
	void seek(Time tpos);
	void audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount);

	float master_gain_get() const { return m_master_gain; }
	void master_gain_set(float gain) { m_master_gain = gain; }

	void filter_get(float &f_lp, float &f_hp);
	void filter_set(float f_lp, float f_hp);
	
	float get_pitch() const { return m_pitch; }
	void set_pitch(float pitch) { m_pitch = std::clamp(pitch, 0.01f, 100.0f); }

	float get_stretch() const { return m_stretch; }
	void set_stretch(float stretch) { m_stretch = std::clamp(stretch, 0.01f, 100.0f); }


	std::vector<Channel>& get_channels() { return m_channels; }

private:
	Streams &m_streams;	
	Samplerate m_srate;
	std::vector<Channel> m_channels;
	std::atomic<Time> m_play_pos{0};
	SDL_AudioStream *m_sdl_audio_stream{nullptr};
	size_t m_idx{};
	size_t m_idx_prev{};
	size_t m_xfade{};
	uint32_t m_t_event{};
	size_t m_frames_event{};
	size_t m_frame_size;
	size_t m_buf_frames;
	std::atomic<float> m_master_gain{1.0f};
	std::atomic<float> m_pitch{1.0f};
	std::atomic<float> m_stretch{1.0f};
	std::vector<float> m_buf{};
	struct {
		std::atomic<float> f_hp{0.0f};
		std::atomic<float> f_lp{1.0f};
		Biquad bq_hp[2];
		Biquad bq_lp[2];
	} m_filter;
};
