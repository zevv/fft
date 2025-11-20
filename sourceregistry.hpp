
#pragma once

#include <SDL3/SDL_audio.h>

#include "source.hpp"

class SourceRegistry {
public:
	SourceRegistry(Source::Info info);
	static Source *create(const char *name, SDL_AudioSpec &dst_spec, char *args);
	static std::vector<Source::Info> &registry();
};


#define REGISTER_STREAM_READER(class, ...) \
	static Source::Info reg = { \
		__VA_ARGS__ \
		.fn_new = [](SDL_AudioSpec &dst_spec, char *args) -> Source* { return new class(reg, dst_spec, args); }, \
	}; \
	static SourceRegistry info(reg);
