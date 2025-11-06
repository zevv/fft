
#include <time.h>
#include <math.h>
#include <stdio.h>


#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"

double hirestime()
{
	struct timespec ts;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
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
	snprintf(note, note_len, "%s%d", name[nr], oct);
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
