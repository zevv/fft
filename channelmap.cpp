
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "channelmap.hpp"
#include "style.hpp"


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


std::generator<int> ChannelMap::enabled_channels()
{
	for (size_t i=0; i<m_channel_count; ++i) {
		if (m_map & (1<<i)) {
			co_yield i;
		}
	}
}


bool ChannelMap::is_channel_enabled(int channel)
{
	return (m_map & (1 << channel)) != 0;
}


void ChannelMap::draw()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));

	for(size_t i=0; i<m_channel_count; i++) {
		if(i > 0) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		ImGui::SameLine();
		ImVec4 col = m_map & (1 << i)
			? Style::channel_color(i)
			: Style::color(Style::ColorId::ChannelDisabled);

		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);

		char label[32];
		snprintf(label, sizeof(label), "%zu", i+1);
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

