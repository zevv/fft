#pragma once

#include <vector>
#include <stddef.h>
#include <mutex>

class Stream {

public:
	Stream();
	void set_size(size_t size);
	void push(float v);
	float get(size_t idx);
	void get(size_t idx, float *out, size_t count);

private:

	size_t m_size;
	size_t m_head;
	size_t m_tail;
	std::vector<float> m_data;
};


class Streams {
public:
	Streams();
	void push(size_t channel, float v);
	float get(size_t channel, size_t idx);
	void get(size_t channel, size_t idx, float *out, size_t count);
	std::vector<Stream> m_streams;
};
