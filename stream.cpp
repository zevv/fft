#include <string.h>
#include "stream.hpp"


Streams::Streams()
	: m_depth(1024 * 1024 * 1024)
	, m_used(0)
	  , m_channels(8)
{
	m_rb.set_size(m_channels * m_depth * sizeof(Sample));
}

void Streams::write(void *data, size_t nframes)
{
	size_t n = nframes * m_channels * sizeof(Sample);
	m_rb.write(data, n);
	m_used += nframes;
	if(m_used > m_depth - 1) {
		m_used = m_depth -1;
	}
}

Sample *Streams::peek(size_t channel, size_t &stride, size_t *used)
{
	stride = m_channels;
	if(used) *used = m_used;
	Sample *data = (Sample *)m_rb.peek();
	return &data[channel];
}

