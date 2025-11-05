
#include <time.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"

double hirestime()
{
	struct timespec ts;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
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
