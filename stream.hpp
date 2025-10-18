#pragma once

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <mutex>

#include "rb.hpp"


class Streams {
public:

	Streams()
	{
		m_channels = 8;
		m_rb.set_size(m_channels * 1024 * 1024 * sizeof(float));
	}

	void write(void *data, size_t nframes)
	{
		size_t n = nframes * m_channels * sizeof(float);
		m_rb.write(data, n);
	}

	float *peek(size_t channel, size_t idx, size_t &stride)
	{
		stride = m_channels;
		return (float *)m_rb.peek(sizeof(float) * ((idx + 1) * m_channels - channel));
	}

private:
	size_t m_channels;
	Rb m_rb;
};


