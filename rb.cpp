
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/types.h>

#include "rb.hpp"

Rb::Rb()
{}

Rb::~Rb()
{
	clear();
}


void Rb::set_size(size_t size)
{
	clear();

	// round up to multiple of page size
	size_t page_size = sysconf(_SC_PAGE_SIZE);
	size = (size + page_size - 1) & ~(page_size - 1);
	m_size = size;

	// create memfd and set its size
	m_fd = memfd_create("rb", MFD_CLOEXEC);
	assert(m_fd != -1);
	int r = ftruncate(m_fd, size * 2);
	assert(r == 0);

	// map two regions side by side
	void *addr1 = mmap(nullptr, size * 2, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	void *addr2 = (uint8_t *)addr1 + size;
	assert(addr1 != MAP_FAILED);
	m_map1 = (uint8_t *)mmap(addr1, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, m_fd, 0);
	assert(m_map1 != MAP_FAILED);
	m_map2 = (uint8_t *)mmap(addr2, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, m_fd, 0);
	assert(m_map2 != MAP_FAILED);
}


void Rb::clear()
{
	m_size = 0;
	m_head = 0;
	if(m_fd != -1) {
		close(m_fd);
		m_fd = -1;
	}
	if(m_map1 != nullptr) {
		munmap(m_map1, m_size);
		m_map1 = nullptr;
	}
	if(m_map2 != nullptr) {
		munmap(m_map2, m_size);
		m_map2 = nullptr;
	}
}


void Rb::write(void *data, size_t len)
{
	assert(m_map1 != nullptr);
	assert(m_size > 0);
	assert(len < m_size);
	memcpy(m_map1 + m_head, data, len);
	m_head = (m_head + len) % m_size;
	m_used += len;
	if(m_used > m_size) m_used = m_size;
}


void *Rb::peek(size_t *used)
{
	if(used) *used = m_used;
	size_t off = m_head + m_size - m_used;
	return m_map1 + off;
}


