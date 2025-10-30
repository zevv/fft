
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "stream-reader-file.hpp"

StreamReaderFile::StreamReaderFile(size_t ch_count, int fd)
	: StreamReader("fd", ch_count)
	, m_fd(fd)
{
	fcntl(m_fd, F_SETFL, O_NONBLOCK);
}


StreamReaderFile::~StreamReaderFile()
{
	if(m_fd != -1) {
		::close(m_fd);
		m_fd = -1;
	}
}


void StreamReaderFile::poll()
{
	if(m_fd == -1) return;

	size_t bytes_max;
	void *buf = m_rb.get_write_ptr(&bytes_max);
	if(bytes_max == 0) return;

	ssize_t r = read(m_fd, buf, bytes_max);
	if(r > 0) {
		m_rb.write_done(r);
	} else {
		if(r < 0) {
			fprintf(stderr, "StreamReaderFile read error: %s\n", strerror(errno));
		}
		::close(m_fd);
		m_fd = -1;
	}
}

