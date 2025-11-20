
#include <imgui.h>

#include "misc.hpp"
#include "vumeter.hpp"


void VuMeter::update(Sample *buf, size_t nframes, size_t stride)
{

}

void VuMeter::draw(void)
{

	float width = 80;
	float height = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
	ImVec2 pos1, pos2;
	ImGui::SetNextItemWidth(width);
	float val = 0.0;
	ImGui::DragFloat("##" UNIQUE_ID, &val, 0.0f, 0.0f, 1.0f, "");

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetItemRectMin();
	pos.y += 4;
	pos.x += 4;

	float db_range = 60.0f;
	float db = 20.0f * log10f((float)m_peak + 1e-10f);
	float db_clamped = std::clamp(db, -db_range, 0.0f);
	float x = (db_clamped + db_range) / db_range * width;

	for(float i=0; i<x-8; i+=8) {
		draw_list->AddRectFilled(ImVec2(pos.x + i, pos.y), ImVec2(pos.x + i + 4, pos.y + height - 8),
			IM_COL32(64, 64, 64, 255));
	}

	draw_list->AddRectFilled(ImVec2(pos.x + x - 8, pos.y), ImVec2(pos.x + x - 4, pos.y + height - 8),
		IM_COL32((uint8_t)(m_col.x * 255.0f), (uint8_t)(m_col.y * 255.0f), (uint8_t)(m_col.z * 255.0f), 255));

}


