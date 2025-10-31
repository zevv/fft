
#include <math.h>

#include "types.hpp"
#include "stream.hpp"


static void audio_callback_(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	StreamPlayer *player = (StreamPlayer *)userdata;
	player->audio_callback(stream, additional_amount, total_amount);
}


StreamPlayer::StreamPlayer(Streams &streams)
	: m_streams(streams)
{
	SDL_AudioSpec fmt{};
	fmt.freq = 48000;
	fmt.format = SDL_AUDIO_F32;
	fmt.channels = 1;
	m_sdl_audio_stream = SDL_OpenAudioDeviceStream(            
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			&fmt,
			audio_callback_,
			(void *)this);
}


void StreamPlayer::seek(Time t)
{
	m_play_pos = t;
}


void StreamPlayer::audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	float buffer[1024]{};
	double srate = 48000.0;

	size_t frame_count = total_amount / sizeof(float);
	if(frame_count > 1024) frame_count = 1024;

	size_t stride = 0;
	size_t avail = 0;
	Sample *data = m_streams.peek(&stride, &avail);

	if(m_xfade <= 0.0) {
		Time delta = fabs(m_play_pos - m_idx / srate);
		if(delta > 0.01) {
			m_idx_prev = m_idx;
			m_idx = m_play_pos * srate;
			m_xfade = 1.0;
		}
	}

	for(size_t i=0; i<frame_count; i++) {
		float v = 0.0;
		if(m_idx >= 0 && m_idx < avail) {
			v = data[m_idx * stride] / (float)k_sample_max;
		}
		if(m_xfade > 0.0) {
			float v_prev = 0.0;
			if(m_idx_prev >= 0 && m_idx_prev < avail) {
				v_prev = data[m_idx_prev * stride] / (float)k_sample_max;
			}
			v = v_prev * m_xfade + v * (1.0 - m_xfade);
			m_xfade -= 1.0 / (srate * 0.050);
		}

		buffer[i] = v;
		m_idx ++;
		m_idx_prev ++;
	}

	SDL_PutAudioStreamData(m_sdl_audio_stream, buffer, frame_count * sizeof(float));
	m_play_pos += (Time)frame_count / srate;
	
	uint64_t t_now = SDL_GetTicks();
	if(t_now > m_t_event) {
		m_t_event = t_now + 10;
		SDL_Event event;
		SDL_zero(event);
		event.type = SDL_EVENT_USER;
		event.user.code = k_user_event_audio_playback;
		event.user.data1 = (void *)m_idx;
		event.user.data2 = (void *)frame_count;
		SDL_PushEvent(&event);
	}
}


void StreamPlayer::enable(bool enable)
{
	auto dev = SDL_GetAudioStreamDevice(m_sdl_audio_stream);
	(enable ? SDL_ResumeAudioDevice : SDL_PauseAudioDevice)(dev);
	SDL_ClearAudioStream(m_sdl_audio_stream);
}


