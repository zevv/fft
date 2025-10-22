#include <string.h>
#include "stream.hpp"


Streams::Streams()
	: m_depth(1024 * 1024 * 1024)
	, m_channels(8)
{
	m_rb.set_size(m_channels * m_depth * sizeof(Sample));
}

void Streams::write(void *data, size_t nframes)
{
	size_t n = nframes * m_channels * sizeof(Sample);
	m_rb.write(data, n);
}

Sample *Streams::peek(size_t channel, size_t *stride, size_t *frames_avail)
{
	size_t bytes_used;
	Sample *data = (Sample *)m_rb.peek(&bytes_used);
	if(stride) *stride = m_channels;
	if(frames_avail) *frames_avail = bytes_used / (m_channels * sizeof(Sample));
	return &data[channel];
}

