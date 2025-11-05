
#include "stream.hpp"
#include "wavecache.hpp"

Wavecache::Wavecache(size_t step)
	: m_step(step)
{
}


void Wavecache::allocate(size_t depth, size_t channel_count)
{
	m_channel_count = channel_count;
	m_frame_size = channel_count * sizeof(Range);
	m_n = 0;
	m_rb.set_size(depth * m_frame_size / m_step);
}


Wavecache::Range *Wavecache::peek(size_t *frames_avail, size_t *stride)
{
	size_t bytes_used;
	Range *data = (Range *)m_rb.peek(&bytes_used);
	if(frames_avail) *frames_avail = bytes_used / m_frame_size;
	if(stride) *stride = m_channel_count;
	return data;
}


void Wavecache::feed_frames(Sample *buf, size_t frame_count, size_t channel_count)
{
	Range *pout = (Range *)m_rb.get_write_ptr();
	size_t frames_out = 0;
	for(size_t i=0; i<frame_count; i++) {
		for(size_t ch=0; ch<channel_count; ch++) {
			const int8_t v = buf[ch] / 255;
			pout[ch].min = m_n == 0 ? v : std::min(pout[ch].min, v);
			pout[ch].max = m_n == 0 ? v : std::max(pout[ch].max, v);
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



