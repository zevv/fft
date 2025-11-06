

#pragma once

double hirestime();
void freq_to_note(double freq, char *note, size_t note_len);

namespace ImGui {
	bool ToggleButton(const char* str_id, bool* v);
	bool IsMouseInRect(const SDL_Rect& r);
}


