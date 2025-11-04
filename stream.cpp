#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>
#include <assert.h>
#include <experimental/simd>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "stream.hpp"
#include "stream-reader.hpp"

Streams::Streams()
	: player(StreamPlayer(*this))
	, m_wavecache(Wavecache(256))
{
}


Streams::~Streams()
{
	m_capture.running = false;
	m_capture.thread.join();

	player.enable(false);

	for(auto reader : m_readers) {
		delete reader;
	}
}


void Streams::load(ConfigReader::Node *n)
{
	player.load(n);
}


void Streams::save(ConfigWriter &cw)
{
	player.save(cw);
}


void Streams::allocate(size_t depth)
{
	m_frame_size = m_channel_count * sizeof(Sample);
	m_depth = depth;
	m_rb.set_size(m_depth * m_frame_size);
	m_wavecache.allocate(depth, m_channel_count);
	m_capture_buf.resize(m_channel_count * 4096);
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
	player.set_channel_count(m_channel_count);
}



void Streams::capture_enable(bool enable)
{
	if(enable && !m_capture.running) {
		m_capture.running = true;
		m_capture.thread = std::thread(&Streams::capture_thread, this);
	}
	for(auto &reader : m_readers) {
		reader->open();
	}
	m_capture.enabled = enable;
}


void Streams::capture_thread()
{
	uint64_t t_event = SDL_GetTicks() + 10;

	while(m_capture.running) {

		while(!m_capture.enabled && m_capture.running) {
			usleep(10000);
		}
	
		// poll all readers
		for(auto reader : m_readers) {
			reader->poll();
		}

		// calculate common lowest number of available frames
		size_t frame_count = m_capture_buf.size() / m_channel_count;
		for(auto reader : m_readers) {
			SDL_AudioStream *sas = reader->get_sdl_audio_stream();
			int bytes_avail = SDL_GetAudioStreamAvailable(sas);
			frame_count = std::min(frame_count, (size_t)bytes_avail / reader->frame_size());
		}

		// collect available frames from all readers into stream buffer
		size_t channel = 0;
		if(frame_count > 0) {

			size_t bytes_max = 0;
			Sample *buf = (Sample *)m_rb.get_write_ptr(&bytes_max);

			for(auto &reader : m_readers) {
				// read data from reader SDL audio stream
				size_t reader_channel_count = reader->channel_count();
				SDL_AudioStream *sas = reader->get_sdl_audio_stream();
				int bytes_want = frame_count * reader_channel_count * sizeof(Sample);
				int bytes_read = SDL_GetAudioStreamData(sas, m_capture_buf.data(), bytes_want);
				assert(bytes_read >= 0);

				// deinterleave into ring buffer
				Sample *p_src = m_capture_buf.data();
				Sample *p_dst = buf + channel;
				size_t src_stride = reader_channel_count;
				size_t dst_stride = m_channel_count;
				for(size_t i=0; i<frame_count; i++) {
					memcpy(p_dst, p_src, reader_channel_count * sizeof(Sample));
					p_src += src_stride;
					p_dst += dst_stride;
				}

				channel += reader_channel_count;
			}

			// update ring buffer write pointer and wavecache
			m_rb.write_done(frame_count * m_frame_size);
			m_wavecache.feed_frames(buf, frame_count, m_channel_count);

			// signal main thread new audio is available
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


