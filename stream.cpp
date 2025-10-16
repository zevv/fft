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
    if(idx > m_size) {
        idx = m_size;
    }
    if(idx + count > m_size) {
        count = m_size - idx;
    }
    if (count == 0) {
        return;
    }

    size_t i = m_head + m_size - idx - 1;
    if(i >= m_size) i -= m_size;
    size_t start_index = (i + m_size - count + 1) % m_size;

    if (start_index <= i) {
        memcpy(out, &m_data[start_index], count * sizeof(float));
    } else {
        size_t first_block_size = m_size - start_index;
        memcpy(out, &m_data[start_index], first_block_size * sizeof(float));
        size_t second_block_size = count - first_block_size;
        memcpy(out + first_block_size, &m_data[0], second_block_size * sizeof(float));
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
	
StreamsReader::StreamsReader(Streams &streams, int channel_base, int channel_count)
	: m_streams(streams)
	, m_channel_base(channel_base)
	, m_channel_count(channel_count)
	, m_frame_bytes(sizeof(float) * channel_count)
	, m_pos(0)
{}


void StreamsReader::handle_data(uint8_t *data, size_t nbytes)
{
	// try to complete incomplete frame
	if(m_pos > 0) {
		size_t n = std::min(nbytes, m_frame_bytes - m_pos);
		memcpy(buf + m_pos, data, n);
		m_pos += n;
		nbytes -= n;
		data += n;
		if(m_pos == m_frame_bytes) {
			float *f = (float *)buf;
			for(int i=0; i<m_channel_count; i++) {
				m_streams.get(m_channel_base + i).write(*f++);
			}
			m_pos = 0;
		}
	}

	// handle full frames
	while(nbytes >= m_frame_bytes) {
		float *f = (float *)data;
		for(int i=0; i<m_channel_count; i++) {
			m_streams.get(m_channel_base + i).write(*f++);
		}
		nbytes -= m_frame_bytes;
		data += m_frame_bytes;
	}

	// store remainder
	if(nbytes > 0) {
		memcpy(buf, data, nbytes);
		m_pos = nbytes;
	}
}


