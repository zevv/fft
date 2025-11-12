
#include <math.h>

#include "types.hpp"
#include "stream.hpp"


static void audio_callback_(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	StreamPlayer *player = (StreamPlayer *)userdata;
	player->audio_callback(stream, additional_amount, total_amount);
}


StreamPlayer::StreamPlayer(Stream &stream)
	: m_stream(stream)
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


void StreamPlayer::filter_get(float &f_lp, float &f_hp)
{
	f_lp = m_filter.f_lp;
	f_hp = m_filter.f_hp;
}


void StreamPlayer::filter_set(float f_lp, float f_hp)
{
	if(f_lp != m_filter.f_lp || f_hp != m_filter.f_hp) {

		m_filter.f_lp = f_lp;
		m_filter.f_hp = f_hp;

		for(size_t i=0; i<2; i++) {
			for(size_t j=0; j<2; j++) {
				m_filter.bq_hp[i][j].configure(Biquad::Type::HP, f_hp);
				m_filter.bq_lp[i][j].configure(Biquad::Type::LP, f_lp);
			}
		}
	}

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

	float master_gain = 1.0f;
	nc->read("master_gain", master_gain);
	m_master_gain = master_gain;

	float filter_lp = 1.0f;
	float filter_hp = 0.0f;
	nc->read("filter_lp", filter_lp);
	nc->read("filter_hp", filter_hp);
	filter_set(filter_lp, filter_hp);

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
	cw.write("master_gain", m_master_gain);
	cw.write("filter_lp", m_filter.f_lp);
	cw.write("filter_hp", m_filter.f_hp);
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
	m_channels.resize(m_stream.channel_count());

	size_t frame_count = total_amount / m_frame_size;
	if(frame_count > m_buf_frames) frame_count = m_buf_frames;

	size_t stride = 0;
	size_t avail = 0;
	Sample *data = m_stream.peek(&stride, &avail);

	std::vector<float> gain[2];
	gain[0].resize(m_stream.channel_count());
	gain[1].resize(m_stream.channel_count());

	for(size_t ch=0; ch<m_stream.channel_count(); ch++) {
		gain[0][ch] = m_channels[ch].gain * (m_channels[ch].pan <= 0.0f ? 1.0f : (1.0f - m_channels[ch].pan));
		gain[1][ch] = m_channels[ch].gain * (m_channels[ch].pan >= 0.0f ? 1.0f : (1.0f + m_channels[ch].pan));
	}

	size_t xfade_samples = m_srate * 0.030 * m_pitch;
	float factor = m_stretch / m_pitch;

	for(size_t i=0; i<frame_count; i++) {

		Time delta = fabs(m_play_pos - (Time(m_idx) / m_srate));
		if(delta > 0.020 || m_stretch != 1.0f) {
			if(m_xfade == 0) {
				m_idx_prev = m_idx;
				m_idx = m_play_pos * m_srate;
				m_xfade = xfade_samples;
			}
		}

		float v[2]{};
		float g0 = (float)m_xfade / (float)xfade_samples;
		float g1 = (1.0f - g0);

		for(size_t ch=0; ch<m_stream.channel_count(); ch++) {
			if(m_channels[ch].enabled && m_idx >= 0 && m_idx < avail) {
				float v_ch = data[m_idx * stride + ch] / (float)k_sample_max;
				if(m_xfade > 0) {
					if(m_idx_prev >= 0 && m_idx_prev < avail) {
						float v_prev = data[m_idx_prev * stride + ch] / (float)k_sample_max;
						v_ch = v_prev * g0 + v_ch * g1;
					}
				}
				for(size_t lr=0; lr<2; lr++) {
					v[lr] += v_ch * gain[lr][ch] * m_master_gain;
				}
			}
		}

		if(m_xfade > 0) {
			m_xfade --;
		}

		for(size_t lr=0; lr<2; lr++) {
			for(size_t j=0; j<2; j++) {
				if(m_filter.f_hp > 0.0f) v[lr] = m_filter.bq_hp[0][j].run(v[lr]);
				if(m_filter.f_lp < 1.0f) v[lr] = m_filter.bq_lp[0][j].run(v[lr]);
			}
			m_buf[i*2 + lr] = v[lr];
		}

		m_idx ++;
		m_idx_prev ++;
		m_play_pos += 1.0 / m_srate * factor; 
	}

	SDL_SetAudioStreamFrequencyRatio(m_sdl_audio_stream, m_pitch);
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


void StreamPlayer::resume()
{
	SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_audio_stream));
	SDL_ClearAudioStream(m_sdl_audio_stream);
}


void StreamPlayer::pause()
{
	SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(m_sdl_audio_stream));
	SDL_ClearAudioStream(m_sdl_audio_stream);
}

