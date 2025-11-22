
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "style.hpp"


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


void sdl_audiospec_to_str(SDL_AudioSpec &spec, char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%s:%d:%d",
			sdl_audioformat_to_str(spec.format),
			(int)spec.channels,
			(int)spec.freq);
}


Gain db_to_gain(Db db)
{
	return pow(10.0, db / 20.0);
}


Db gain_to_db(Gain gain)
{
	if (gain <= 0.0) {
		return -90.0;
	}
	return 20.0 * log10(gain);
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


void humanize(double val, char *buf, size_t buf_len)
{
	struct scale {
		double v;
		const char *suffix;
	} scales[] = {
		{ 1e12, "T" },
		{ 1e9,  "G" },
		{ 1e6,  "M" },
		{ 1e3,  "k" },
		{ 1.0,  "" },
		{ 1e-3, "m" },
		{ 1e-6, "μ" },
		{ 1e-9, "n" },
		{ 1e-12,"p" },
		{ 0.0,  nullptr }
	};

	for(int i=0; scales[i].suffix != nullptr; i++) {
		if(fabs(val) >= scales[i].v * 0.99) {
			double v = val / scales[i].v;
			snprintf(buf, buf_len, "%.5g %s", v, scales[i].suffix);
			return;
		}
	}
}



void bitline(const char *fmt, ...)
{
#ifdef BITLINE
	static int fd = -1;
	if(fd == -1) {
		fd = open("/tmp/bitline.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	}

	va_list args;
	va_start(args, fmt);
	
	char buf[256];
	int l = 0;
	l += snprintf(buf+l, sizeof(buf)-l, "%f ", hirestime());
	l += vsnprintf(buf+l, sizeof(buf)-l, fmt, args);
	l += snprintf(buf+l, sizeof(buf)-l, "\n");

	write(fd, buf, l);
	
	va_end(args);
#endif
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
	if(midi_number < 0 || midi_number > 127) {
		snprintf(note, note_len, "---");
		return;
	}
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
	ImVec4 col = Style::color(*v ? Style::ColorId::ToggleButtonOn : Style::ColorId::ToggleButtonOff);
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
    
void TextShadow(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	ImVec2 text_size = ImGui::CalcTextSize(buf);
	auto draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
			ImVec2(ImGui::GetCursorScreenPos().x + text_size.x,
			ImGui::GetCursorScreenPos().y + text_size.y),
			IM_COL32(0, 0, 0, 128), 0.0f);
	ImGui::TextUnformatted(buf);

}


bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format, int flags)
{
	float v_f = static_cast<float>(*v);
	bool changed = ImGui::SliderFloat(label, &v_f, static_cast<float>(v_min), static_cast<float>(v_max), format, flags);
	if(changed) *v = static_cast<double>(v_f);
	return changed;
}


bool DragDouble(const char* label, double* v, double v_speed, double v_min, double v_max, const char* format, int flags)
{
	float v_f = static_cast<float>(*v);
	bool changed = ImGui::DragFloat(label, &v_f, static_cast<float>(v_speed), static_cast<float>(v_min), static_cast<float>(v_max), format, flags);
	if(changed) *v = static_cast<double>(v_f);
	return changed;
}


bool IsKeyChordDown(int key)
{

	auto &io = ImGui::GetIO();
	if((key & ImGuiMod_Shift) && !io.KeyShift) return false;
	if((key & ImGuiMod_Ctrl) && !io.KeyCtrl) return false;
	if((key & ImGuiMod_Alt) && !io.KeyAlt) return false;

	key &= ~ImGuiMod_Mask_;

	return ImGui::IsKeyDown((ImGuiKey)key);
}

	
}

