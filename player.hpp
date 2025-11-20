
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
	
	struct Channel {
		bool enabled{true};
		float gain{0.3f};
		float pan{0.0f};
	};

	Player(Stream &stream);
	~Player();
	
	void load(ConfigReader::Node *n);
	void save(ConfigWriter &cw);

	void set_channel_count(size_t count);
	void set_sample_rate(Samplerate srate);
	void pause();
	void resume();
	void seek(Time tpos);
	void audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount);

	float master_gain_get() const { return m_master_gain; }
	void master_gain_set(float gain) { m_master_gain = gain; }

	void filter_get(float &f_lp, float &f_hp);
	void filter_set(float f_lp, float f_hp);
	
	float pitch() const { return m_pitch; }
	void set_pitch(float pitch) { m_pitch = std::clamp(pitch, 0.01f, 100.0f); }

	Frequency shift() const { return m_shiftfreq; }
	void set_shift(Frequency shift) { m_shiftfreq = shift; }

	float stretch() const { return m_stretch; }
	void set_stretch(float stretch) { m_stretch = std::clamp(stretch, 0.01f, 100.0f); }


	std::vector<Channel>& channel_count() { return m_channels; }

private:
	Stream &m_stream;	
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
		Biquad bq_hp[2][2];
		Biquad bq_lp[2][2];
	} m_filter;
	FreqShift m_freqshift[2];
	std::atomic<Frequency> m_shiftfreq{0.0};
};
