
#pragma once

#include <stdint.h>

class Rb {

public:
	Rb();
	~Rb();
	void set_size(size_t size);
	void write(void *data, size_t len);
	void *peek(size_t idx);
	void dump();

private:

	void clear();

	int m_fd;
	size_t m_size;
	size_t m_head;
	uint8_t *m_map1;
	uint8_t *m_map2;
};
