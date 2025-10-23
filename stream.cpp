#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>

#include <SDL3/SDL_audio.h>

#include "stream.hpp"


Streams::Streams()
	: m_depth(1024 * 1024)
	, m_channels(8)
	, m_frame_size(m_channels * sizeof(Sample))
{
	m_rb.set_size(m_depth * m_frame_size);
}


Streams::~Streams()
{
	for(auto reader : m_readers) {
		delete reader;
	}
}



Sample *Streams::peek(size_t channel, size_t *stride, size_t *frames_avail)
{
	size_t bytes_used;
	Sample *data = (Sample *)m_rb.peek(&bytes_used);
	if(stride) *stride = m_channels;
	if(frames_avail) *frames_avail = bytes_used / m_frame_size;
	return &data[channel];
}


void Streams::add_reader(StreamReader *reader) {
	m_readers.push_back(reader);
}


void Streams::capture()
{
	size_t frame_count = SIZE_MAX;
	for(auto reader : m_readers) {
		reader->poll();
		frame_count = std::min(frame_count, reader->frames_avail());
	}
	
	for(auto reader : m_readers) {
		printf("%s:", reader->name());
		size_t frames_avail = reader->frames_avail();
		for(size_t i=0; i<4096; i+=512) {
			putchar((frames_avail > i) ? '#' : '.');
		}
		printf(" ");
	}
	printf("\n");

	if(frame_count > 0) {
		Sample *buf = (Sample *)m_rb.get_write_ptr();
		for(auto reader : m_readers) {
			reader->drain_into(buf, frame_count, m_channels);
		}
		m_rb.write_done(frame_count * m_frame_size);
	}
}


StreamReader::StreamReader(const char *name, size_t ch_start, size_t ch_count)
	: m_name(name)
	, m_ch_start(ch_start)
	, m_ch_count(ch_count)
	, m_frame_size(ch_count * sizeof(Sample))
{
	m_rb.set_size(m_frame_size * 4096);
}


StreamReader::~StreamReader()
{
	printf("StreamReader %s destroyed\n", m_name);
}


size_t StreamReader::frames_avail()
{
	return m_rb.bytes_used() / m_frame_size;
}



void StreamReader::drain_into(Sample *p_dst, size_t frame_count, size_t dst_stride)
{
	Sample *p_src = (Sample *)m_rb.read(frame_count * m_frame_size);
	size_t src_stride = m_ch_count;

	for(size_t i=0; i<frame_count; i++) {
		memcpy(p_dst + m_ch_start, p_src, m_frame_size);
		p_src += src_stride;
		p_dst += dst_stride;
	}
}



StreamReaderFd::StreamReaderFd(size_t ch_start, size_t ch_count, int fd)
	: StreamReader("fd", ch_start, ch_count)
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
	printf("StreamReaderFd destroyed\n");
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


StreamReaderAudio::StreamReaderAudio(size_t ch_start, size_t ch_count, float srate)
	: StreamReader("audio", ch_start, ch_count)
{
	SDL_AudioSpec want{};
    want.freq = srate;
    want.format = SDL_AUDIO_F32;
    want.channels = 2;

    m_sdl_audiostream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &want, nullptr, (void *)this);

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_audiostream));
	fprintf(stderr, "StreamReaderAudio::StreamReaderAudio %p\n", m_sdl_audiostream);
}


StreamReaderAudio::~StreamReaderAudio()
{
	fprintf(stderr, "StreamReaderAudio::~StreamReaderAudio %p\n", m_sdl_audiostream);
	//if(m_sdl_audiostream) {
	//	SDL_DestroyAudioStream(m_sdl_audiostream);
	//	m_sdl_audiostream = nullptr;
	//}
}


void StreamReaderAudio::poll()
{
	size_t read_max = m_rb.bytes_free();
	if(read_max == 0) return;

	size_t bytes_per_frame = m_ch_count * sizeof(Sample);
	size_t frames_max = read_max / bytes_per_frame;
	size_t bytes_to_read = frames_max * bytes_per_frame;

	void *buf = m_rb.get_write_ptr(&read_max);
	int r = SDL_GetAudioStreamData(m_sdl_audiostream, buf, bytes_to_read);
	if(r > 0) {
		m_rb.write_done(r);
	}
}


StreamReaderGenerator::StreamReaderGenerator(size_t ch_start, size_t ch_count, float srate, int type)
	: StreamReader("gen", ch_start, ch_count)
	, m_type(type)
	, m_srate(srate)
	, m_phase(0.0)
{
}


StreamReaderGenerator::~StreamReaderGenerator()
{
	printf("StreamReaderGenerator destroyed\n");
}


void StreamReaderGenerator::poll()
{
	size_t bytes_gen = m_rb.bytes_free();
	size_t frames_gen = bytes_gen / m_frame_size;
	if(frames_gen == 0) return;

	Sample *buf = (Sample *)m_rb.get_write_ptr(&bytes_gen);
	for(size_t i=0; i<frames_gen; i++) {
		buf[i] = run();
	}
	m_rb.write_done(frames_gen * m_frame_size);
}


Sample StreamReaderGenerator::run()
{
	Sample v = sin(2.0 * M_PI * 440.0 * m_phase);
	m_phase += 1.0 / m_srate;
	if(m_phase >= 1.0) m_phase -= 1.0;
	return v;
}
