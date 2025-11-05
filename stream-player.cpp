
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
	, m_srate(8000)
	, m_frame_size(2 * sizeof(float))
	, m_buf_frames(4096)
{
	SDL_AudioSpec fmt{};
	fmt.freq = m_srate;
	fmt.format = SDL_AUDIO_F32;
	fmt.channels = 2;
	m_sdl_audio_stream = SDL_OpenAudioDeviceStream(            
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			&fmt,
			audio_callback_,
			(void *)this);

	m_buf.resize(m_buf_frames * sizeof(float) * fmt.channels);
}


StreamPlayer::~StreamPlayer()
{
	if(m_sdl_audio_stream) {
		SDL_DestroyAudioStream(m_sdl_audio_stream);
	}
}


void StreamPlayer::set_sample_rate(Samplerate srate)
{
	m_srate = srate;
	SDL_AudioSpec src_spec, dst_spec;
	SDL_GetAudioStreamFormat(m_sdl_audio_stream, &src_spec, &dst_spec);
	src_spec.freq = m_srate;
	SDL_SetAudioStreamFormat(m_sdl_audio_stream, &src_spec, &dst_spec);

	SDL_GetAudioStreamFormat(m_sdl_audio_stream, &src_spec, &dst_spec);
	printf("audio rate %d -> %d\n", src_spec.freq, dst_spec.freq);
}


void StreamPlayer::set_channel_count(size_t count)
{
	m_channels.resize(std::max(count, m_channels.size()));
}

	
void StreamPlayer::load(ConfigReader::Node *n)
{
	auto nc = n->find("player");
	float stretch = 1.0f;
	nc->read("stretch", stretch);
	m_stretch = stretch;

	float pitch = 1.0f;
	nc->read("pitch", pitch);
	m_pitch = pitch;

	for(auto &k : nc->find("channel")->kids) {
		if(isdigit(k.first[0])) {
			size_t ch = atoi(k.first);
			m_channels.resize(ch+1);
			k.second->read("enabled", m_channels[ch].enabled);
			k.second->read("volume", m_channels[ch].gain);
			k.second->read("pan", m_channels[ch].pan);
		}
	}
}


void StreamPlayer::save(ConfigWriter &cw)
{
	cw.push("player");
	cw.write("stretch", m_stretch);
	cw.write("pitch", m_pitch);
	cw.push("channel");
	for(size_t ch=0; ch<m_channels.size(); ch++) {
		cw.push(ch);
		cw.write("enabled", m_channels[ch].enabled);
		cw.write("volume", m_channels[ch].gain);
		cw.write("pan", m_channels[ch].pan);
		cw.pop();
	}
	cw.pop();
	cw.pop();
}


void StreamPlayer::seek(Time t)
{
	m_play_pos = t;
	SDL_Event event{};
	event.type = SDL_EVENT_USER;
	event.user.code = k_user_event_audio_playback;
	event.user.data1 = (void *)(size_t)(t * m_srate);
	event.user.data2 = (void *)0;
	SDL_PushEvent(&event);
}


void StreamPlayer::audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	m_channels.resize(m_streams.channel_count());

	size_t frame_count = total_amount / m_frame_size;
	if(frame_count > m_buf_frames) frame_count = m_buf_frames;

	size_t stride = 0;
	size_t avail = 0;
	Sample *data = m_streams.peek(&stride, &avail);

	std::vector<float> gain_l(m_streams.channel_count());
	std::vector<float> gain_r(m_streams.channel_count());

	for(size_t ch=0; ch<m_streams.channel_count(); ch++) {
		gain_l[ch] = m_channels[ch].gain * (m_channels[ch].pan <= 0.0f ? 1.0f : (1.0f - m_channels[ch].pan));
		gain_r[ch] = m_channels[ch].gain * (m_channels[ch].pan >= 0.0f ? 1.0f : (1.0f + m_channels[ch].pan));
	}

	SDL_SetAudioStreamFrequencyRatio(m_sdl_audio_stream, m_pitch);

	size_t xfade_samples = m_srate * 0.030 * m_pitch;
	float factor = m_stretch / m_pitch;

	for(size_t i=0; i<frame_count; i++) {

		float vl = 0.0;
		float vr = 0.0;

		Time delta = fabs(m_play_pos - (Time(m_idx) / m_srate));
		if(delta > 0.020 || m_stretch != 1.0f) {
			if(m_xfade == 0) {
				m_idx_prev = m_idx;
				m_idx = m_play_pos * m_srate;
				m_xfade = xfade_samples;
			}
		}

		float g0 = (float)m_xfade / (float)xfade_samples;
		float g1 = 1.0f - g0;

		for(size_t ch=0; ch<m_streams.channel_count(); ch++) {

			if(m_channels[ch].enabled == false) continue;

			float v = 0.0f;
			if(m_idx >= 0 && m_idx < avail) {
				v = data[m_idx * stride + ch] / (float)k_sample_max;
			}
			if(m_xfade > 0) {
				if(m_idx_prev >= 0 && m_idx_prev < avail) {
					float v_prev = data[m_idx_prev * stride + ch] / (float)k_sample_max;
					v = v_prev * g0 + v * g1;
				}
			}

			vl += v * gain_l[ch];
			vr += v * gain_r[ch];
		}

		if(m_xfade > 0) {
			m_xfade --;
		}

		m_buf[i*2 + 0] = vl;
		m_buf[i*2 + 1] = vr;
		m_idx ++;
		m_idx_prev ++;
		m_play_pos += 1.0 / m_srate * factor; 
	}

	SDL_PutAudioStreamData(m_sdl_audio_stream, m_buf.data(), frame_count * m_frame_size);
	m_frames_event += frame_count * factor;
	
	uint64_t t_now = SDL_GetTicks();
	if(t_now > m_t_event) {
		m_t_event = t_now + 10;
		SDL_Event event{};
		event.type = SDL_EVENT_USER;
		event.user.code = k_user_event_audio_playback;
		event.user.data1 = (void *)m_idx;
		event.user.data2 = (void *)m_frames_event;
		SDL_PushEvent(&event);
		m_frames_event = 0;
	}
}


void StreamPlayer::enable(bool enable)
{
	auto dev = SDL_GetAudioStreamDevice(m_sdl_audio_stream);
	(enable ? SDL_ResumeAudioDevice : SDL_PauseAudioDevice)(dev);
	SDL_ClearAudioStream(m_sdl_audio_stream);
}
