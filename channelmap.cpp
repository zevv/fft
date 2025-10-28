
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "channelmap.hpp"



SDL_Color ChannelMap::ch_color(int channel)
{
	// https://medialab.github.io/iwanthue/
	static const SDL_Color color[] = {
		{  82, 202,  59, 255 },
		{ 222,  85, 231, 255 },
		{  68, 124, 254, 255 },
		{ 244,  74,  54, 255 },
		{ 175, 198,  31, 255 },
		{ 163, 102, 248, 255 },
		{ 242,  62, 173, 255 },
		{ 239, 157,  22, 255 },
	};
	if(channel >= 0 && channel < 8) {
		return color[channel];
	} else {
		return SDL_Color{ 255, 255, 255, 255 };
	}
}


void ChannelMap::load(ConfigReader::Node *node)
{
	node->read("channel_map", m_map);
}


void ChannelMap::save(ConfigWriter &cw)
{
	cw.write("channel_map", m_map);
}


void ChannelMap::set_channel_count(int count)
{
	m_channel_count = count;
}


bool ChannelMap::ch_enabled(int channel)
{
	if(channel >= 0 && (size_t)channel < m_channel_count) {
		return (m_map & (1 << channel)) != 0;
	} else {
		return false;
	}
}


void ChannelMap::ch_set(int channel, bool enabled)
{
	if(channel >= 0 && (size_t)channel < m_channel_count) {
		if(enabled) {
			m_map |= (1 << channel);
		} else {
			m_map &= ~(1 << channel);
		}
	}
}


void ChannelMap::ch_toggle(int channel)
{
	if(channel >= 0 && (size_t)channel < m_channel_count) {
		m_map ^= (1 << channel);
	}
}


void ChannelMap::draw()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));

	for(size_t i=0; i<m_channel_count; i++) {
		if(i > 0) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		}

		ImGui::SameLine();
		SDL_Color c = ch_color(i);
		ImVec4 col = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		if(ch_enabled(i)) {
			col = ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
		}
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		char label[2] = { (char)('1' + i), 0 };
		ImGuiKey key = (ImGuiKey)(ImGuiKey_1 + i);
		if(ImGui::Button(label) || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(key))) {
			if(ImGui::GetIO().KeyShift) m_map = 0;
			ch_toggle(i);
		}
		ImGui::PopStyleColor(3);
		if(i > 0) ImGui::PopStyleVar(1);
	}
	ImGui::PopStyleVar(1);
}

