#pragma once

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <mutex>

#include "rb.hpp"
#include "types.hpp"


class Streams {
public:

	Streams();
	void write(void *data, size_t nframes);
	Sample *peek(size_t channel, size_t *stride, size_t *used = nullptr);

private:
	size_t m_depth;
	size_t m_channels;
	Rb m_rb;
};


