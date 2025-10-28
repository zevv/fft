#pragma once

#include <stdio.h>
#include <vector>
#include <stddef.h>
#include <mutex>

#include "rb.hpp"
#include "types.hpp"


class Wavecache {
public:
	struct Range {
		int8_t min;
		int8_t max;
	};

	Wavecache(size_t step);
	void allocate(size_t depth, size_t channel_count);
	Range *peek(size_t *frames_avail, size_t *stride);
	void feed_frames(Sample *buf, size_t frame_count, size_t channel_count);
private:
	size_t m_channel_count;
	size_t m_frame_size;
	size_t m_step;
	size_t m_n;
	Rb m_rb;
};


class StreamReader;

class Streams {
public:

	Streams();
	~Streams();
	size_t channel_count() { return m_channel_count; }
	void allocate(size_t depth);
	Sample *peek(size_t *stride, size_t *used = nullptr);
	Wavecache::Range *peek_wavecache(size_t *stride, size_t *used = nullptr);
	void add_reader(StreamReader *reader);
	bool capture();

private:
	size_t m_depth{};
	size_t m_channel_count{};
	size_t m_frame_size{};
	Rb m_rb;
	Wavecache m_wavecache;
	std::vector<StreamReader *> m_readers{};
};



class StreamReader {
public:
	StreamReader(const char *name, size_t ch_count);
	virtual ~StreamReader();
	size_t frames_avail();
	size_t channel_count() { return m_ch_count; }
	size_t drain_into(Sample *buf, size_t ch_start, size_t frame_count, size_t stride);
	virtual void poll() = 0;
	const char *name() { return m_name; }
protected:
	const char *m_name;
	size_t m_ch_count;
	size_t m_frame_size{};
	Rb m_rb;
};


class StreamReaderFd : public StreamReader {
public:
	StreamReaderFd(size_t ch_count, int fd);
	~StreamReaderFd();
	void poll() override;
private:
	int m_fd{-1};
};


class StreamReaderAudio : public StreamReader {
public:
	StreamReaderAudio(size_t ch_count, float srate);
	~StreamReaderAudio();
	void poll() override;
private:
	SDL_AudioStream *m_sdl_audiostream{};
};


class StreamReaderGenerator : public StreamReader {
public:

	StreamReaderGenerator(size_t ch_count, float srate, int type);
	~StreamReaderGenerator();
	void poll() override;
private:

	Sample run();
	Samplerate m_srate{};
	Time m_phase{};
	int m_type{};
};

