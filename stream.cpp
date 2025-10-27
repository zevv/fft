#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>
#include <experimental/simd>

#include <SDL3/SDL_audio.h>

#include "stream.hpp"


Streams::Streams(size_t depth, size_t channel_count)
	: m_depth(depth)
	, m_channels(channel_count)
	, m_frame_size(m_channels * sizeof(Sample))
	, m_wavecache(Wavecache(depth, channel_count, 256))
{
	m_rb.set_size(m_depth * m_frame_size);
}


Streams::~Streams()
{
	for(auto reader : m_readers) {
		delete reader;
	}
}



Sample *Streams::peek(size_t *stride, size_t *frames_avail)
{
	size_t bytes_used;
	Sample *data = (Sample *)m_rb.peek(&bytes_used);
	if(stride) *stride = m_channels;
	if(frames_avail) *frames_avail = bytes_used / m_frame_size;
	return data;
}


SampleRange *Streams::peek_wavecache(size_t *stride, size_t *frames_avail)
{
	return m_wavecache.peek(frames_avail, stride);
}


void Streams::add_reader(StreamReader *reader) {
	m_readers.push_back(reader);
}


bool Streams::capture()
{
	bool captured = false;
	size_t frame_count = SIZE_MAX;
	for(auto reader : m_readers) {
		reader->poll();
		frame_count = std::min(frame_count, reader->frames_avail());
	}

	if(false) {
		for(auto reader : m_readers) {
			printf("%s:", reader->name());
			size_t frames_avail = reader->frames_avail();
			for(size_t i=0; i<4096; i+=512) {
				putchar((frames_avail > i) ? '#' : '.');
			}
			printf(" ");
		}
		printf("\n");
	}

	size_t bytes_max = 0;
	Sample *buf = (Sample *)m_rb.get_write_ptr(&bytes_max);

	size_t channel = 0;
	if(frame_count > 0) {
		for(auto reader : m_readers) {
			channel += reader->drain_into(buf, channel, frame_count, m_channels);
		}
		m_rb.write_done(frame_count * m_frame_size);
		m_wavecache.feed_frames(buf, frame_count, m_channels);
		captured = true;
	}
	return captured;
}


Wavecache::Wavecache(size_t depth, size_t channel_count, size_t step)
	: m_channel_count(channel_count)
	, m_frame_size(channel_count * sizeof(SampleRange))
	, m_step(step)
	, m_n(0)
{
	m_rb.set_size(depth * m_frame_size / step);
}


SampleRange *Wavecache::peek(size_t *frames_avail, size_t *stride)
{
	size_t bytes_used;
	SampleRange *data = (SampleRange *)m_rb.peek(&bytes_used);
	if(frames_avail) *frames_avail = bytes_used / m_frame_size;
	if(stride) *stride = m_channel_count;
	return data;
}


void Wavecache::feed_frames(Sample *buf, size_t frame_count, size_t channel_count)
{
	SampleRange *pout = (SampleRange *)m_rb.get_write_ptr();
	size_t frames_out = 0;
	for(size_t i=0; i<frame_count; i++) {
		if(m_n == 0) {
			for(size_t ch=0; ch<channel_count; ch++) {
				pout[ch].min = buf[ch];
				pout[ch].max = buf[ch];
			}
		} else {
			for(size_t ch=0; ch<channel_count; ch++) {
				pout[ch].min = std::min(pout[ch].min, buf[ch]);
				pout[ch].max = std::max(pout[ch].max, buf[ch]);
			}
		}
		m_n ++;
		if(m_n == m_step) {
			pout += channel_count;
			frames_out ++;
			m_n = 0;
		}
		buf += channel_count;
	}
	m_rb.write_done(m_frame_size * frames_out);
}


StreamReader::StreamReader(const char *name, size_t ch_count)
	: m_name(name)
	, m_ch_count(ch_count)
	, m_frame_size(ch_count * sizeof(Sample))
{
	m_rb.set_size(4096);
}


StreamReader::~StreamReader()
{
}


size_t StreamReader::frames_avail()
{
	return m_rb.bytes_used() / m_frame_size;
}



size_t StreamReader::drain_into(Sample *p_dst, size_t ch_start, size_t frame_count, size_t dst_stride)
{
	Sample *p_src = (Sample *)m_rb.read(frame_count * m_frame_size);
	size_t src_stride = m_ch_count;

	for(size_t i=0; i<frame_count; i++) {
		memcpy(p_dst + ch_start, p_src, m_frame_size);
		p_src += src_stride;
		p_dst += dst_stride;
	}

	return m_ch_count;
}



StreamReaderFd::StreamReaderFd(size_t ch_count, int fd)
	: StreamReader("fd", ch_count)
	, m_fd(fd)
{
	fcntl(m_fd, F_SETFL, O_NONBLOCK);
}


StreamReaderFd::~StreamReaderFd()
{
	if(m_fd != -1) {
		::close(m_fd);
		m_fd = -1;
	}
}


void StreamReaderFd::poll()
{
	if(m_fd == -1) return;

	size_t bytes_max;
	void *buf = m_rb.get_write_ptr(&bytes_max);
	if(bytes_max == 0) return;

	ssize_t r = read(m_fd, buf, bytes_max);
	if(r > 0) {
		m_rb.write_done(r);
	} else {
		if(r < 0) {
			fprintf(stderr, "StreamReaderFd read error: %s\n", strerror(errno));
		}
		::close(m_fd);
		m_fd = -1;
	}
}


StreamReaderAudio::StreamReaderAudio(size_t ch_count, float srate)
	: StreamReader("audio", ch_count)
{
	SDL_AudioSpec fmt{};
    fmt.freq = srate;
#ifdef SAMPLE_S16
	fmt.format = SDL_AUDIO_S16;
#else
    fmt.format = SDL_AUDIO_F32;
#endif
    fmt.channels = ch_count;

	SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &fmt, nullptr, (void *)this);

	if(stream) {
		m_sdl_audiostream = stream;
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
	} else {
		fprintf(stderr, "StreamReaderAudio: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
	}
}


StreamReaderAudio::~StreamReaderAudio()
{
	if(m_sdl_audiostream) {
		SDL_DestroyAudioStream(m_sdl_audiostream);
	}
	m_sdl_audiostream = nullptr;
}


void StreamReaderAudio::poll()
{
	if(!m_sdl_audiostream) return;
	size_t read_max;
	void *buf = m_rb.get_write_ptr(&read_max);
	if(read_max == 0) return;

	size_t bytes_per_frame = m_ch_count * sizeof(Sample);
	size_t frames_max = read_max / bytes_per_frame;
	size_t bytes_to_read = frames_max * bytes_per_frame;

	int r = SDL_GetAudioStreamData(m_sdl_audiostream, buf, bytes_to_read);
	if(r > 0) {
		m_rb.write_done(r);
	}
}


StreamReaderGenerator::StreamReaderGenerator(size_t ch_count, float srate, int type)
	: StreamReader("gen", ch_count)
	, m_srate(srate)
	, m_phase(0.0)
	, m_type(type)
{
}


StreamReaderGenerator::~StreamReaderGenerator()
{
}


void StreamReaderGenerator::poll()
{
	size_t bytes_gen;
	Sample *buf = (Sample *)m_rb.get_write_ptr(&bytes_gen);
	size_t frames_gen = bytes_gen / m_frame_size;
	if(frames_gen == 0) return;

	for(size_t i=0; i<frames_gen; i++) {
		buf[i] = run();
	}
	m_rb.write_done(frames_gen * m_frame_size);
}


Sample StreamReaderGenerator::run()
{
	Sample v = 0;

	if(m_type == 0) {
		v = sin(2.0 * M_PI * 440.0 * m_phase);
		m_phase += 1.0 / m_srate;
		if(m_phase >= 1.0) m_phase -= 1.0;
	}

	if(m_type == 1) {
		v = m_phase - 0.5;
		m_phase += 440.0 / m_srate;
		if(m_phase > 1.0) m_phase -= 1.0;
	}

	return v;
}
