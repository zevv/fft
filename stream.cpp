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


Streams::Streams()
	: m_wavecache(Wavecache(256))
{
}


Streams::~Streams()
{
	m_running = false;
	m_thread.join();
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


void Streams::capture_thread()
{
	uint64_t t_event = SDL_GetTicks() + 10;

	while(m_running) {

		while(!m_enabled && m_running) {
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


void Streams::capture_enable(bool enable)
{
	if(enable && !m_running) {
		m_running = true;
		m_thread = std::thread(&Streams::capture_thread, this);
	}
	m_enabled = enable;
}


