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
#include "source.hpp"

Stream::Stream()
	: player(Player(*this))
	, capture(*this, m_rb, m_wavecache)
	, m_wavecache(Wavecache(256))
{
}


Stream::~Stream()
{
	player.pause();
	capture.pause();
}


void Stream::load(ConfigReader::Node *n)
{
	player.load(n);
}


void Stream::save(ConfigWriter &cw)
{
	player.save(cw);
}


void Stream::set_sample_rate(Samplerate srate)
{
	m_srate = srate;
	capture.set_sample_rate(srate);
	player.set_sample_rate(srate);
}


void Stream::allocate(size_t depth)
{
	m_channel_count = capture.channel_count();

	m_frame_size = m_channel_count * sizeof(Sample);
	m_depth = depth;
	m_rb.set_size(m_depth * m_frame_size);
	m_wavecache.allocate(depth, m_channel_count);
	player.set_channel_count(m_channel_count);
}


Sample *Stream::peek(size_t *stride, size_t *frames_avail)
{
	size_t bytes_used;
	Sample *data = (Sample *)m_rb.peek(&bytes_used);
	if(stride) *stride = m_channel_count;
	if(frames_avail) *frames_avail = bytes_used / m_frame_size;
	return data;
}


Wavecache::Range *Stream::peek_wavecache(size_t *stride, size_t *frames_avail)
{
	return m_wavecache.peek(frames_avail, stride);
}



