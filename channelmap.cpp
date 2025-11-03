
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "channelmap.hpp"


// https://medialab.github.io/iwanthue/
static const SDL_Color color[] = {
	{  58, 186,  42, 255 },
	{ 198,  69, 214, 255 },
	{  68, 124, 254, 255 },
	{ 244,  74,  54, 255 },
	{ 175, 198,  31, 255 },
	{ 163, 102, 248, 255 },
	{ 242,  62, 173, 255 },
	{ 239, 157,  22, 255 },
};


SDL_Color ShowChannelMap::ch_color(int channel)
{
	if(channel >= 0 && channel < 8) {
		return color[channel];
	} else {
		return SDL_Color{ 255, 255, 255, 255 };
	}
}


void ShowChannelMap::load(ConfigReader::Node *node)
{
	node->read("channel_map", m_map);
}


void ShowChannelMap::save(ConfigWriter &cw)
{
	cw.write("channel_map", m_map);
}


void ShowChannelMap::set_channel_count(int count)
{
	m_channel_count = count;
}


std::generator<int> ShowChannelMap::enabled_channels()
{
	for (size_t i=0; i<m_channel_count; ++i) {
		if (m_map & (1<<i)) {
			co_yield i;
		}
	}
}


bool ShowChannelMap::is_channel_enabled(int channel)
{
	return (m_map & (1 << channel)) != 0;
}


void ShowChannelMap::draw()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));

	for(size_t i=0; i<m_channel_count; i++) {
		if(i > 0) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		ImGui::SameLine();
		SDL_Color c = ch_color(i);
		ImVec4 col = m_map & (1 << i)
			? ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f)
			: ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);

		char label[2] = { (char)('1' + i), 0 };
		ImGuiKey key = (ImGuiKey)(ImGuiKey_1 + i);
		if(ImGui::Button(label) || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(key))) {
			if(ImGui::GetIO().KeyShift) m_map = 0;
			m_map ^= (1 << i);
		}

		ImGui::PopStyleColor(3);
		if(i > 0) ImGui::PopStyleVar(1);
	}
	ImGui::PopStyleVar(1);
}

