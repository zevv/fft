#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>
#include <assert.h>
#include <wordexp.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "stream.hpp"
#include "stream-capture.hpp"
#include "stream-reader-file.hpp"
#include "stream-reader-generator.hpp"
#include "stream-reader-audio.hpp"
#include "stream-reader-jack.hpp"


StreamCapture::StreamCapture(Streams &streams, Rb &rb, Wavecache &wavecache) 
	: m_streams(streams)
	, m_rb(rb)
	, m_wavecache(wavecache)
	, m_spec{SDL_AUDIO_S16, 1, 8000}
{
}


StreamCapture::~StreamCapture()
{
	enable(false);
	for(auto reader : m_readers) {
		delete reader;
	}
}


void StreamCapture::set_sample_rate(Samplerate srate)
{
	m_spec.freq = srate;
}

void StreamCapture::enable(bool enable)
{
	size_t channel_count = 0;
	for(auto &reader : m_readers) {
		reader->open();
		channel_count += reader->channel_count();
	}

	m_enabled = enable;
	m_spec.channels = channel_count;
	m_buf.resize(channel_count * 4096);

	if(enable && !m_running) {
		m_running = true;
		m_thread = std::thread(&StreamCapture::capture_thread, this);
	}

	if(!enable && m_running) {
		m_running = false;
		if(m_thread.joinable()) {
			m_thread.join();
		}
	}
}


static struct {
	const char *name;
	SDL_AudioFormat format;
} audio_format_map[] = {
	{ "u8",     SDL_AUDIO_U8 },
	{ "s8",     SDL_AUDIO_S8 },
	{ "s16",    SDL_AUDIO_S16 },
	{ "s16le",  SDL_AUDIO_S16LE },
	{ "s16be",  SDL_AUDIO_S16BE },
	{ "s32",    SDL_AUDIO_S32 },
	{ "s32le",  SDL_AUDIO_S32LE },
	{ "s32be",  SDL_AUDIO_S32BE },
	{ "f32",    SDL_AUDIO_F32 },
	{ "f32le",  SDL_AUDIO_F32LE },
	{ "f32be",  SDL_AUDIO_F32BE },
	{ nullptr },
};

SDL_AudioSpec parse_audio_spec(char *s)
{
	SDL_AudioSpec fmt;
	fmt.format = SDL_AUDIO_S16LE;
	fmt.channels = 1;
	fmt.freq = 48000;

	while(s) {
		char *endptr;
		double val = strtod(s, &endptr);
		if(endptr != s) {
			if(val <= 32) fmt.channels = val;
			if(val >  32) fmt.freq = val;
		} else {
			for(int i=0; audio_format_map[i].name != nullptr; i++) {
				if(strcmp(s, audio_format_map[i].name) == 0) {
					fmt.format = audio_format_map[i].format;
				}
			}
		}
		s = strtok(nullptr, ":");
	}

	return fmt;
}


void StreamCapture::add_reader(const char *desc)
{
	char *desc_copy = strdup(desc);
	
	const char *type = strtok(desc_copy, ":");
	
	if(strcmp(type, "raw") == 0) {
		const char *fname = strtok(nullptr, ":");
		wordexp_t p;
		if(wordexp(fname, &p, 0) == 0) {
			fname = p.we_wordv[0];
			SDL_AudioSpec src_spec = parse_audio_spec(strtok(nullptr, ":"));
			int fd = open(fname, O_RDONLY);
			if(fd > 0) {
				SDL_AudioSpec dst_spec = m_spec;
				dst_spec.channels = src_spec.channels;
				StreamReader *reader = new StreamReaderFile(dst_spec, src_spec, fd);
				m_readers.push_back(reader);
			} else {
				fprintf(stderr, "open('%s'): %s\n", fname, strerror(errno));
			}
			wordfree(&p);
		}
	}

	if(strcmp(type, "stdin") == 0) {
		SDL_AudioSpec src_spec = parse_audio_spec(strtok(nullptr, ":"));
		SDL_AudioSpec dst_spec = m_spec;
		dst_spec.channels = src_spec.channels;
		StreamReader *reader = new StreamReaderFile(dst_spec, src_spec, 0);
		m_readers.push_back(reader);
	}
	
	if(strcmp(type, "gen") == 0) {
		int type = atoi(strtok(nullptr, ":"));
		StreamReader *reader = new StreamReaderGenerator(m_spec, type);
		m_readers.push_back(reader);
	}

	if(strcmp(type, "audio") == 0) {
		SDL_AudioSpec src_spec = parse_audio_spec(strtok(nullptr, ":"));
		SDL_AudioSpec dst_spec = m_spec;
		dst_spec.channels = src_spec.channels;
		StreamReader *reader = new StreamReaderAudio(dst_spec);
		m_readers.push_back(reader);
	}
	
	if(strcmp(type, "jack") == 0) {
		SDL_AudioSpec src_spec = parse_audio_spec(strtok(nullptr, ":"));
		SDL_AudioSpec dst_spec = m_spec;
		dst_spec.channels = src_spec.channels;
		StreamReader *reader = new StreamReaderJack(dst_spec);
		m_readers.push_back(reader);
	}

	free(desc_copy);
}


size_t StreamCapture::channel_count()
{
	size_t channel_count = 0;
	for(auto &reader : m_readers) {
		channel_count += reader->channel_count();
	}
	return channel_count;
}


void StreamCapture::capture_thread()
{
	uint64_t t_event = SDL_GetTicks() + 10;

	while(m_running) {

		while(!m_enabled && m_running) {
			usleep(10000);
		}
	
		// poll all readers
		for(auto reader : m_readers) {
			SDL_AudioStream *sas = reader->get_sdl_audio_stream();
			int bytes_queued = SDL_GetAudioStreamQueued(sas);
			if(bytes_queued < 64000) {
				reader->poll();
			}
		}

		// calculate common lowest number of available frames
		size_t frame_count = m_buf.size() / m_streams.channel_count();
		for(auto reader : m_readers) {
			SDL_AudioStream *sas = reader->get_sdl_audio_stream();
			int bytes_avail = SDL_GetAudioStreamAvailable(sas);
			if(bytes_avail < 0) {
				printf("SDL_GetAudioStreamAvailable(): %s\n", SDL_GetError());
				exit(1);
			}
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
				int bytes_read = SDL_GetAudioStreamData(sas, m_buf.data(), bytes_want);
				//assert(bytes_read >= 0);

				// deinterleave into ring buffer
				Sample *p_src = m_buf.data();
				Sample *p_dst = buf + channel;
				size_t src_stride = reader_channel_count;
				size_t dst_stride = m_streams.channel_count();
				for(size_t i=0; i<frame_count; i++) {
					memcpy(p_dst, p_src, reader_channel_count * sizeof(Sample));
					p_src += src_stride;
					p_dst += dst_stride;
				}

				channel += reader_channel_count;
			}

			// update ring buffer write pointer and wavecache
			m_rb.write_done(frame_count * m_streams.channel_count() * sizeof(Sample));
			m_wavecache.feed_frames(buf, frame_count, m_streams.channel_count());

			// signal main thread new audio is available
			uint64_t t_now = SDL_GetTicks();
			if(t_now > t_event) {
				t_event += 10;
				SDL_Event event;
				SDL_zero(event);
				event.type = SDL_EVENT_USER;
				event.user.code = k_user_event_audio_capture;
				event.user.data1 = (void *)(m_rb.bytes_used() / (m_streams.channel_count() * sizeof(Sample)));
				SDL_PushEvent(&event);
			}
		} else {
			usleep(1000);
		}
	}
}
