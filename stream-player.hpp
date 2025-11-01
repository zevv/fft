
#pragma once

#include <atomic>

#include <SDL3/SDL.h>


class Streams;

class StreamPlayer {
public:
	StreamPlayer(Streams &streams);
	~StreamPlayer();
	void enable(bool onoff);
	void seek(Time tpos);
	void audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount);

private:
	Streams &m_streams;	
	Samplerate m_srate;
	std::atomic<Time> m_play_pos{0};
	SDL_AudioStream *m_sdl_audio_stream{nullptr};
	size_t m_idx{};
	size_t m_idx_prev{};
	double m_xfade{};
	uint32_t m_t_event{};
};


