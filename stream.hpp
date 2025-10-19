#pragma once

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <mutex>

#include "rb.hpp"


class Streams {
public:

	Streams()
		: m_depth(1024 * 1024 * 1024)
		, m_used(0)
		, m_channels(8)
	{
		m_rb.set_size(m_channels * m_depth * sizeof(float));
	}

	void write(void *data, size_t nframes)
	{
		size_t n = nframes * m_channels * sizeof(float);
		m_rb.write(data, n);
		m_used += nframes;
		if(m_used > m_depth - 1) {
			m_used = m_depth -1;
		}
	}

	float *peek(size_t channel, size_t idx, size_t &stride, size_t *used = nullptr)
	{
		stride = m_channels;
		if(used) *used = m_used;
		return (float *)m_rb.peek(sizeof(float) * ((idx + 1) * m_channels - channel));
	}

private:
	size_t m_depth;
	size_t m_used;
	size_t m_channels;
	Rb m_rb;
};


