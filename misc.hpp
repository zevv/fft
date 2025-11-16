

#pragma once

#include <vector>
#include <math.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "types.hpp"

const char *sdl_audioformat_to_str(SDL_AudioFormat sdl_audioformat);
SDL_AudioFormat sdl_audioformat_from_str(const char *s);
SDL_AudioSpec sdl_audiospec_from_str(char *s);
void duration_to_str(Time duration, char *buf, size_t buf_len);

double hirestime();
void freq_to_note(double freq, char *note, size_t note_len);
void bitline(const char *fmt, ...);

void humanize(double val, char *buf, size_t buf_len);

namespace ImGui {
	bool ToggleButton(const char* str_id, bool* v);
	bool IsMouseInRect(const SDL_Rect& r);
    void TextShadow(const char* fmt, ...);
}


template<typename T>
T tabread2(const std::vector<T>& vs, double pos, T v_oob = T{})
{
    if (pos < 0.0 || pos > 1.0) {
        return v_oob;
    }
    if (vs.empty()) {
        return v_oob;
    }
    if (pos == 1.0) {
        return vs.back();
    }
    if (vs.size() == 1) {
        return vs[0];
    }
    double p = pos * static_cast<double>(vs.size() - 1);
    size_t i = static_cast<size_t>(p);
    size_t j = i + 1;
    double a0 = p - static_cast<double>(i);
    double a1 = 1.0 - a0;
    const T& v0 = vs[i];
    const T& v1 = vs[j];
    return (v0 * a1) + (v1 * a0);
}
