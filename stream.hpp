#pragma once

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <mutex>

#include "rb.hpp"
#include "types.hpp"


class StreamReader;

class Streams {
public:

	Streams();
	~Streams();
	Sample *peek(size_t channel, size_t *stride, size_t *used = nullptr);
	void add_reader(StreamReader *reader);
	bool capture();

private:
	size_t m_depth{};
	size_t m_channels{};
	size_t m_frame_size{};
	Rb m_rb;
	std::vector<StreamReader *> m_readers{};
};



class StreamReader {
public:
	StreamReader(const char *name, size_t ch_start, size_t ch_count);
	virtual ~StreamReader();
	size_t frames_avail();
	void drain_into(Sample *buf, size_t frame_count, size_t stride);
	virtual void poll() = 0;
	const char *name() { return m_name; }
protected:
	const char *m_name;
	size_t m_ch_start;
	size_t m_ch_count;
	size_t m_frame_size{};
	Rb m_rb;
};


class StreamReaderFd : public StreamReader {
public:
	StreamReaderFd(size_t ch_start, size_t ch_count, int fd);
	~StreamReaderFd();
	void poll() override;
private:
	int m_fd{-1};
};


class StreamReaderAudio : public StreamReader {
public:
	StreamReaderAudio(size_t ch_start, size_t ch_count, float srate);
	~StreamReaderAudio();
	void poll() override;
private:
	SDL_AudioStream *m_sdl_audiostream{};
};


class StreamReaderGenerator : public StreamReader {
public:

	StreamReaderGenerator(size_t ch_start, size_t ch_count, float srate, int type);
	~StreamReaderGenerator();
	void poll() override;
private:

	Sample run();
	int m_type{};
	float m_srate{};
	double m_phase{};
};

