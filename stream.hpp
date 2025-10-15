#pragma once

#include <vector>
#include <stddef.h>
#include <mutex>

class Stream {

public:
	Stream();
	void set_size(size_t size);
	void write(float v);
	float read(size_t idx);
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


class StreamsReader {

	StreamsReader(Streams &streams, int channel_map);
	void handle_data(void *data, size_t nbytes);

private:
	Streams m_streams;
	int m_channel_map;

};
