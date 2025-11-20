
#include <math.h>

#include "types.hpp"
#include "stream.hpp"


static void audio_callback_(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	Player *player = (Player *)userdata;
	player->audio_callback(stream, additional_amount, total_amount);
}


Player::Player(Stream &stream)
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


Player::~Player()
{
	if(m_sdl_audio_stream) {
		SDL_DestroyAudioStream(m_sdl_audio_stream);
	}
}


void Player::set_sample_rate(Samplerate srate)
{
	m_srate = srate;
	SDL_AudioSpec src_spec, dst_spec;
	SDL_GetAudioStreamFormat(m_sdl_audio_stream, &src_spec, &dst_spec);
	src_spec.freq = m_srate;
	SDL_SetAudioStreamFormat(m_sdl_audio_stream, &src_spec, &dst_spec);

	SDL_GetAudioStreamFormat(m_sdl_audio_stream, &src_spec, &dst_spec);
	printf("audio rate %d -> %d\n", src_spec.freq, dst_spec.freq);
}


void Player::set_channel_count(size_t count)
{
	m_channel_config.resize(std::max(count, m_channel_config.size()));
}


void Player::load(ConfigReader::Node *n)
{
	auto nc = n->find("player");

	Config cfg;

	nc->read("master_gain", cfg.master);
	nc->read("shift", cfg.shift);
	nc->read("pitch", cfg.pitch);
	nc->read("stretch", cfg.stretch);
	nc->read("filter_hp", cfg.freq_hp);
	nc->read("filter_lp", cfg.freq_lp);

	set_config(cfg);

	for(auto &k : nc->find("channel")->kids) {
		if(isdigit(k.first[0])) {
			size_t ch = atoi(k.first);
			ChannelConfig ccfg;
			k.second->read("enabled", ccfg.enabled);
			k.second->read("level", ccfg.level);
			k.second->read("pan", ccfg.pan);
			set_channel_config(ch, ccfg);
		}
	}
}


void Player::save(ConfigWriter &cw)
{
	cw.push("player");

	Config cfg = config();

	cw.write("master_gain", cfg.master);
	cw.write("shift", cfg.shift);
	cw.write("pitch", cfg.pitch);
	cw.write("stretch", cfg.stretch);
	cw.write("filter_hp", cfg.freq_hp);
	cw.write("filter_lp", cfg.freq_lp);


	cw.push("channel");
	for(size_t ch=0; ch<m_channel_config.size(); ch++) {
		ChannelConfig ccfg = channel_config(ch);
		cw.push(ch);
		cw.write("enabled", ccfg.enabled);
		cw.write("level", ccfg.level);
		cw.write("pan", ccfg.pan);
		cw.pop();
	}
	cw.pop();
	cw.pop();
}


Player::Config Player::config()
{
	return m_config;
}


void Player::set_config(Config &cfg)
{
	cfg.pitch = std::clamp(cfg.pitch, 0.01, 100.0);
	cfg.stretch = std::clamp(cfg.stretch, 0.01, 100.0);

	Config cfg_old = config();
	if(cfg_old != cfg) {

		for(size_t lr=0; lr<2; lr++) {

			m_freqshift[lr].set_frequency(cfg.shift / m_srate);

			for(size_t j=0; j<2; j++) {
				m_filter.bq_hp[lr][j].configure(Biquad::Type::HP, cfg.freq_hp / m_srate);
				m_filter.bq_lp[lr][j].configure(Biquad::Type::LP, cfg.freq_lp / m_srate);
			}
		}
		m_config = cfg;
	}
}


Player::ChannelConfig& Player::channel_config(size_t ch)
{
	if(ch >= m_channel_config.size()) {
		m_channel_config.resize(ch+1);
	}
	return m_channel_config[ch];
}


void Player::set_channel_config(size_t ch, ChannelConfig &channel)
{
	if(ch >= m_channel_config.size()) {
		m_channel_config.resize(ch+1);
	}
	m_channel_config[ch] = channel;
}


void Player::seek(Time t)
{
	m_play_pos = t;
	SDL_Event event{};
	event.type = SDL_EVENT_USER;
	event.user.code = k_user_event_audio_playback;
	event.user.data1 = (void *)(size_t)(t * m_srate);
	event.user.data2 = (void *)0;
	SDL_PushEvent(&event);
}


void Player::audio_callback(SDL_AudioStream *stream, int additional_amount, int total_amount)
{

	Config cfg = config();

	Gain master_gain = db_to_gain(cfg.master);
	m_channel_config.resize(m_stream.channel_count());

	size_t frame_count = total_amount / m_frame_size;
	if(frame_count > m_buf_frames) frame_count = m_buf_frames;

	size_t stride = 0;
	size_t avail = 0;
	Sample *data = m_stream.peek(&stride, &avail);

	// TODO: precalculate

	std::vector<float> gain[2];
	gain[0].resize(m_stream.channel_count());
	gain[1].resize(m_stream.channel_count());

	for(size_t ch=0; ch<m_stream.channel_count(); ch++) {
		ChannelConfig ccfg = channel_config(ch);
		Gain channel_gain = db_to_gain(ccfg.level);
		gain[0][ch] = channel_gain * (ccfg.pan <= 0.0f ? 1.0f : (1.0f - ccfg.pan));
		gain[1][ch] = channel_gain * (ccfg.pan >= 0.0f ? 1.0f : (1.0f + ccfg.pan));
	}

	size_t xfade_samples = m_srate * 0.030 * cfg.pitch;
	float factor = cfg.stretch / cfg.pitch;

	for(size_t i=0; i<frame_count; i++) {

		// handle seeking and crossfade

		Time delta = fabs(m_play_pos - (Time(m_idx) / m_srate));
		if(delta > 0.020 || cfg.stretch != 1.0f) {
			if(m_xfade == 0) {
				m_idx_prev = m_idx;
				m_idx = m_play_pos * m_srate;
				m_xfade = xfade_samples;
			}
		}

		float v[2]{};
		float g0 = (float)m_xfade / (float)xfade_samples;
		float g1 = (1.0f - g0);

		// mix source channels into L/R playback channels

		for(size_t ch=0; ch<m_stream.channel_count(); ch++) {
			if(m_channel_config[ch].enabled && m_idx >= 0 && m_idx < avail) {
				float v_ch = data[m_idx * stride + ch] / (float)k_sample_max;
				if(m_xfade > 0) {
					if(m_idx_prev >= 0 && m_idx_prev < avail) {
						float v_prev = data[m_idx_prev * stride + ch] / (float)k_sample_max;
						v_ch = v_prev * g0 + v_ch * g1;
					}
				}
				for(size_t lr=0; lr<2; lr++) {
					v[lr] += v_ch * gain[lr][ch] * master_gain;
				}
			}
		}

		if(m_xfade > 0) {
			m_xfade --;
		}
	
		// post processing

		for(size_t lr=0; lr<2; lr++) {
		
			if(cfg.shift != 0.0f) {
				v[lr] = m_freqshift[lr].run(v[lr]);
			}

			for(size_t j=0; j<2; j++) {
				if(cfg.freq_hp >     0.0) v[lr] = m_filter.bq_hp[lr][j].run(v[lr]);
				if(cfg.freq_lp < m_srate) v[lr] = m_filter.bq_lp[lr][j].run(v[lr]);
			}
		
			m_buf[i*2 + lr] = v[lr];
		}

		m_idx ++;
		m_idx_prev ++;
		m_play_pos += 1.0 / m_srate * factor; 
	}

	SDL_SetAudioStreamFrequencyRatio(m_sdl_audio_stream, cfg.pitch);
	SDL_PutAudioStreamData(m_sdl_audio_stream, m_buf.data(), frame_count * m_frame_size);
	m_frames_event += frame_count * factor;

	// send playback position event to main thread
	
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


void Player::resume()
{
	SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_audio_stream));
	SDL_ClearAudioStream(m_sdl_audio_stream);
}


void Player::pause()
{
	SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(m_sdl_audio_stream));
	SDL_ClearAudioStream(m_sdl_audio_stream);
}
