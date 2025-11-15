
#include <time.h>
#include <math.h>
#include <stdio.h>


#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"

static struct {
	const char *name;
	SDL_AudioFormat format;
} audio_format_map[] = {
	{ "u8",     SDL_AUDIO_U8 },
	{ "s8",     SDL_AUDIO_S8 },
	{ "s16",    SDL_AUDIO_S16 },
	{ "s16le",  SDL_AUDIO_S16LE },
	{ "s16be",  SDL_AUDIO_S16BE },
	{ "s32",    SDL_AUDIO_S32 },
	{ "s32le",  SDL_AUDIO_S32LE },
	{ "s32be",  SDL_AUDIO_S32BE },
	{ "f32",    SDL_AUDIO_F32 },
	{ "f32le",  SDL_AUDIO_F32LE },
	{ "f32be",  SDL_AUDIO_F32BE },
	{ nullptr },
};


const char *sdl_audioformat_to_str(SDL_AudioFormat sdl_audioformat)
{
	for(int i=0; audio_format_map[i].name != nullptr; i++) {
		if(sdl_audioformat == audio_format_map[i].format) {
			return audio_format_map[i].name;
		}
	}
	return "unknown";
}


SDL_AudioFormat sdl_audioformat_from_str(const char *s)
{
	for(int i=0; audio_format_map[i].name != nullptr; i++) {
		if(strcmp(s, audio_format_map[i].name) == 0) {
			return audio_format_map[i].format;
		}
	}
	return SDL_AUDIO_UNKNOWN;
}


SDL_AudioSpec sdl_audiospec_from_str(char *args)
{
	SDL_AudioSpec fmt;
	fmt.format = SDL_AUDIO_S16LE;
	fmt.channels = 1;
	fmt.freq = 48000;

	char *s = strtok(args, ":");

	while(s) {
		char *endptr;
		double val = strtod(s, &endptr);
		if(endptr != s) {
			if(val <= 32) fmt.channels = val;
			if(val >  32) fmt.freq = val;
		} else {
			SDL_AudioFormat format = sdl_audioformat_from_str(s);
			if(format != SDL_AUDIO_UNKNOWN) {
				fmt.format = format;
			}
		}
		s = strtok(nullptr, ":");
	}

	return fmt;
}



double hirestime()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}


void bitline(const char *fmt, ...)
{
	static FILE *f = nullptr;
	if(f == nullptr) {
		f = fopen("/tmp/bitline.log", "w");
	}

	va_list args;
	va_start(args, fmt);

	if(f) {
		fprintf(f, "%f ", hirestime());
		vfprintf(f, fmt, args);
		fprintf(f, "\n");
		fflush(f);
	}
	
	va_end(args);
}


void freq_to_note(double freq, char *note, size_t note_len)
{
	static const char *name[] = {
		"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
	};
	if(freq <= 0.0) {
		snprintf(note, note_len, "---");
		return;
	}
	int midi_number = static_cast<int>(round(12.0 * log2(freq / 440.0) + 69.0));
	int nr = midi_number % 12;
	int oct = (midi_number / 12) - 1;
	int cents = static_cast<int>(round(1200.0 * log2(freq / (440.0 * pow(2.0, (midi_number - 69) / 12.0)))));
	snprintf(note, note_len, "%s%d %+d", name[nr], oct, cents);
}


void duration_to_str(Time d, char *buf, size_t buf_len)
{
	if(d < 1.0) {
		snprintf(buf, buf_len, "%.0fms", d * 1000.0);
	} else if(d < 60.0) {
		snprintf(buf, buf_len, "%.2fs", d);
	} else if(d < 3600.0) {
		int m = (int)(d / 60.0);
		int s = (int)(d) % 60;
		snprintf(buf, buf_len, "%d:%02d", m, s);
	} else {
		int h = (int)(d / 3600.0);
		int m = ((int)(d) % 3600) / 60;
		int s = (int)(d) % 60;
		snprintf(buf, buf_len, "%d:%02d:%02d", h, m, s);
	}
}


namespace ImGui {
bool ToggleButton(const char* str_id, bool* v)
{
	ImVec4 col = *v ? ImVec4(0.26f, 0.59f, 0.98f, 1.0f) : ImVec4(0.26f, 0.26f, 0.38f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, col);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
	bool pressed = ImGui::Button(str_id);
	if(pressed) *v = !*v;
	ImGui::PopStyleColor(3);
	return pressed;
}


bool IsMouseInRect(SDL_Rect const &rect)
{
	ImVec2 mp = ImGui::GetIO().MousePos;
	return (mp.x >= rect.x) && (mp.x < rect.x + rect.w) &&
	       (mp.y >= rect.y) && (mp.y < rect.y + rect.h);
}

}

