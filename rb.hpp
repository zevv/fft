
#pragma once

#include <stdint.h>

class Rb {

public:
	Rb();
	~Rb();
	void set_size(size_t size);
	size_t size();

	size_t bytes_used();

	void *get_write_ptr(size_t *bytes_max = nullptr);
	void write_done(size_t len);

	void *read(size_t len);
	void *peek(size_t *used = nullptr);


private:

	void clear();

	int m_fd{-1};
	size_t m_size{};
	size_t m_head{};
	size_t m_tail{};
	uint8_t *m_map1{};
	uint8_t *m_map2{};
};
