
#pragma once

#include <stdint.h>

#include "types.hpp"
#include "stream.hpp"

class StreamReader {
public:
	StreamReader(const char *name, size_t ch_count);
	virtual ~StreamReader();
	size_t frames_avail();
	size_t channel_count() { return m_ch_count; }
	size_t drain_into(Sample *buf, size_t ch_start, size_t frame_count, size_t stride);
	virtual void poll() = 0;
	const char *name() { return m_name; }
protected:
	const char *m_name;
	size_t m_ch_count;
	size_t m_frame_size{};
	Rb m_rb;
};


