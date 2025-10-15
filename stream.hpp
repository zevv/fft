#pragma once

#include <vector>
#include <stddef.h>
#include <mutex>

class Stream {

public:
	Stream();
	void set_size(size_t size);
	void write(float v);
	inline float read(size_t idx) {
		float rv = 0.0;
		if(idx < m_size) {
			size_t i = (m_head + m_size - idx - 1) % m_size;
			rv = m_data[i];
		}
		return rv;
	};

	void read(size_t idx, float *out, size_t count);

private:

	size_t m_size;
	size_t m_head;
	size_t m_tail;
	std::vector<float> m_data;
};


class Streams {
public:
	Streams();
	Stream &get(size_t channel);
	std::vector<Stream> m_streams;
};
