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
	size_t i = (m_head + m_size - idx - 1) % m_size;
	return m_data[i];
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

