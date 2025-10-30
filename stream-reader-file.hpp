
#include "stream-reader.hpp"

class StreamReaderFile : public StreamReader {
public:
	StreamReaderFile(size_t ch_count, int fd);
	~StreamReaderFile();
	void poll() override;
private:
	int m_fd{-1};
};

