

#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

const char *sdl_audioformat_to_str(SDL_AudioFormat sdl_audioformat);
SDL_AudioFormat sdl_audioformat_from_str(const char *s);
SDL_AudioSpec sdl_audiospec_from_str(char *s);

double hirestime();
void freq_to_note(double freq, char *note, size_t note_len);

namespace ImGui {
	bool ToggleButton(const char* str_id, bool* v);
	bool IsMouseInRect(const SDL_Rect& r);
}


