#include <string.h>
#include "stream.hpp"

Stream::Stream()
{
	set_size(1024 * 1024);
}


void Stream::set_size(size_t size)
{
	m_size = size;
	m_head = 0;
	m_tail = 0;
	m_data.resize(size);
}


void Stream::write(float v)
{
	m_data[m_head] = v;
	m_head = (m_head + 1) % m_size;
	if(m_head == m_tail) {
		m_tail = (m_tail + 1) % m_size;
	}
}


float Stream::read(size_t idx)
{
	float rv = 0.0;
	if(idx < m_size) {
		size_t i = (m_head + m_size - idx - 1) % m_size;
		rv = m_data[i];
	}
	return rv;
}


void Stream::read(size_t idx, float *out, size_t count)
{
	// fast impl, no calls
	if(idx + count > m_size) {
		count = m_size - idx;
	}
	size_t i = m_head + m_size - idx - 1;
	if(i >= m_size) i -= m_size;
	for(size_t n=0; n<count; n++) {
		out[n] = m_data[i];
		if(i == 0) {
			i = m_size - 1;
		} else {
			i--;
		}
	}
}


Streams::Streams()
{
	for(int i=0; i<8; i++) {
		m_streams.push_back(Stream());
	}
}


Stream &Streams::get(size_t channel)
{
	if(channel < m_streams.size()) {
		return m_streams[channel];
	} else {
		return m_streams[0];
	}
}

StreamsReader::StreamsReader(Streams &streams, int channel_map)
	: m_streams(streams)
	, m_channel_map(channel_map)
{
}


void StreamsReader::handle_data(void *data, size_t nbytes)
{
}
