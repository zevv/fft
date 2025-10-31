
#pragma once

#include <inttypes.h>


class Wavecache {
public:
	struct Range {
		int8_t min;
		int8_t max;
	};

	Wavecache(size_t step);
	void allocate(size_t depth, size_t channel_count);
	Range *peek(size_t *frames_avail, size_t *stride);
	void feed_frames(Sample *buf, size_t frame_count, size_t channel_count);


private:
	size_t m_channel_count;
	size_t m_frame_size;
	size_t m_step;
	size_t m_n;
	Rb m_rb;
};


