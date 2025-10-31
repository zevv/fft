#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>
#include <experimental/simd>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "stream.hpp"
#include "stream-reader.hpp"


static void audio_callback_(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	Streams *streams = (Streams *)userdata;
	streams->playback_callback(stream, additional_amount, total_amount);
}


Streams::Streams()
	: m_wavecache(Wavecache(256))
{
	SDL_AudioSpec fmt{};
	fmt.freq = 48000;
	fmt.format = SDL_AUDIO_F32;
	fmt.channels = 1;
	m_playback.sdl_audio_stream = SDL_OpenAudioDeviceStream(            
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			&fmt,
			audio_callback_,
			(void *)this);

}


Streams::~Streams()
{
	m_capture.running = false;
	m_capture.thread.join();

	for(auto reader : m_readers) {
		delete reader;
	}
}


void Streams::allocate(size_t depth)
{
	m_frame_size = m_channel_count * sizeof(Sample);
	m_depth = depth;
	m_rb.set_size(m_depth * m_frame_size);
	m_wavecache.allocate(depth, m_channel_count);
}


size_t Streams::frames_avail()
{
	return m_rb.bytes_used() / m_frame_size;
}


Sample *Streams::peek(size_t *stride, size_t *frames_avail)
{
	size_t bytes_used;
	Sample *data = (Sample *)m_rb.peek(&bytes_used);
	if(stride) *stride = m_channel_count;
	if(frames_avail) *frames_avail = bytes_used / m_frame_size;
	return data;
}


Wavecache::Range *Streams::peek_wavecache(size_t *stride, size_t *frames_avail)
{
	return m_wavecache.peek(frames_avail, stride);
}


void Streams::add_reader(StreamReader *reader)
{
	m_readers.push_back(reader);
	m_channel_count += reader->channel_count();
}



void Streams::capture_enable(bool enable)
{
	if(enable && !m_capture.running) {
		m_capture.running = true;
		m_capture.thread = std::thread(&Streams::capture_thread, this);
	}
	m_capture.enabled = enable;
}


void Streams::playback_seek(Time t)
{
	m_playback.play_pos = t;
}


void Streams::playback_callback(SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	float buffer[1024]{};
	double srate = 48000.0;

	size_t frame_count = total_amount / sizeof(float);
	if(frame_count > 1024) frame_count = 1024;

	size_t stride = 0;
	size_t avail = 0;
	Sample *data = peek(&stride, &avail);

	Time t1 = m_playback.play_pos.load();
	Time t2 = m_playback.idx / srate;
	Time delta = fabs(t2 - t1);
	if(delta > 0.01) {
		m_playback.idx = t1 * srate;
		m_playback.idx_prev = m_playback.idx;
		m_playback.xfade = 1.0;
	}

	for(size_t i=0; i<frame_count; i++) {
		float v = 0.0;
		if(m_playback.idx >= 0 && m_playback.idx < avail) {
			v = data[m_playback.idx * stride] / (float)k_sample_max;
		}
		if(m_playback.xfade > 0.0) {
			float v_prev = 0.0;
			if(m_playback.idx_prev >= 0 && m_playback.idx_prev < avail) {
				v_prev = data[m_playback.idx_prev * stride] / (float)k_sample_max;
			}
			v = v_prev * m_playback.xfade + v * (1.0 - m_playback.xfade);
			m_playback.xfade -= 1.0 / (srate * 0.005);
		}

		buffer[i] = v;
		m_playback.idx ++;
		m_playback.idx_prev ++;
	}

	SDL_PutAudioStreamData(m_playback.sdl_audio_stream, buffer, frame_count * sizeof(float));
	m_playback.play_pos += (Time)frame_count / srate;
	
	uint64_t t_now = SDL_GetTicks();
	if(t_now > m_playback.t_event) {
		m_playback.t_event = t_now + 10;
		SDL_Event event;
		SDL_zero(event);
		event.type = SDL_EVENT_USER;
		event.user.code = k_user_event_audio_playback;
		event.user.data1 = (void *)m_playback.idx;
		event.user.data2 = (void *)frame_count;
		SDL_PushEvent(&event);
	}
}


void Streams::playback_enable(bool enable)
{
	auto dev = SDL_GetAudioStreamDevice(m_playback.sdl_audio_stream);
	(enable ? SDL_ResumeAudioDevice : SDL_PauseAudioDevice)(dev);
	SDL_ClearAudioStream(m_playback.sdl_audio_stream);
}


void Streams::capture_thread()
{
	uint64_t t_event = SDL_GetTicks() + 10;

	while(m_capture.running) {

		while(!m_capture.enabled && m_capture.running) {
			usleep(10000);
		}

		size_t frame_count = SIZE_MAX;
		for(auto reader : m_readers) {
			reader->poll();
			frame_count = std::min(frame_count, reader->frames_avail());
		}

		size_t bytes_max = 0;
		Sample *buf = (Sample *)m_rb.get_write_ptr(&bytes_max);

		size_t channel = 0;
		if(frame_count > 0) {
			for(auto reader : m_readers) {
				channel += reader->drain_into(buf, channel, frame_count, m_channel_count);
			}
			m_rb.write_done(frame_count * m_frame_size);
			m_wavecache.feed_frames(buf, frame_count, m_channel_count);
		}

		if(frame_count > 0) {
			uint64_t t_now = SDL_GetTicks();
			if(t_now > t_event) {
				t_event += 10;
				SDL_Event event;
				SDL_zero(event);
				event.type = SDL_EVENT_USER;
				event.user.code = k_user_event_audio_capture;
				event.user.data1 = (void *)(m_rb.bytes_used() / m_frame_size);
				SDL_PushEvent(&event);
			}
		} else {
			usleep(1000);
		}
	}
}


