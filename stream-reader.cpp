
#include <string.h>

#include "stream-reader.hpp"


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




