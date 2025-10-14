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


void Stream::push(float v)
{
	m_data[m_head] = v;
	m_head = (m_head + 1) % m_size;
	if(m_head == m_tail) {
		m_tail = (m_tail + 1) % m_size;
	}
}


float Stream::get(size_t idx)
{
	if(idx >= m_size) {
		return 0.0f;
	}
	size_t i = m_head + m_size - idx - 1;
	if(i >= m_size) i -= m_size;
	return m_data[i];
}


void Stream::get(size_t idx, float *out, size_t count)
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


void Streams::push(size_t channel, float v)
{
	if(channel < m_streams.size()) {
		m_streams[channel].push(v);
	}
}
	

float Streams::get(size_t channel, size_t idx)
{
	if(channel < m_streams.size()) {
		return m_streams[channel].get(idx);
	}
	return 0.0f;
}


void Streams::get(size_t channel, size_t idx, float *out, size_t count)
{
	if(channel < m_streams.size()) {
		m_streams[channel].get(idx, out, count);
	} else {
		memset(out, 0, count * sizeof(float));
	}
}

