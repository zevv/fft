
#include <string>
#include <math.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"


static ImVec4 channel_color(int channel)
{
	static const ImVec4 color[] = {
		{ 0.00, 0.75, 0.52, 1.00 },
		{ 0.90, 0.16, 1.00, 1.00 },
		{ 0.00, 0.59, 1.00, 1.00 },
		{ 0.86, 0.49, 0.00, 1.00 },
		{ 1.00, 0.27, 0.33, 1.00 },
		{ 0.56, 0.61, 0.00, 1.00 },
		{ 0.00, 0.70, 0.00, 1.00 },
		{ 1.00, 0.00, 0.84, 1.00 },
	};
	if(channel >= 0 && channel < 8) {
		return color[channel];
	} else {
		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}


Widget::Widget(Type type)
	: m_type(type)
	, m_channel_map{}
	, m_waveform{
		.count = 1024,
		.agc = true,
		.peak = 0.0f,
	}
{
	for(int i=0; i<8; i++) {
		m_channel_map[i] = true;
	}
}


void Widget::draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
{
	// Type selection combo box
	static const char *k_type_str[] = {
		"none", "spectogram", "waterfall", "wave"
	};
	ImGui::SetNextItemWidth(100);
	ImGui::Combo("Type", (int *)&m_type, k_type_str, IM_ARRAYSIZE(k_type_str));

	// Channel enable buttons
	for(size_t i=0; i<8; i++) {
		ImGui::SameLine();
		ImVec4 col = m_channel_map[i] ? channel_color(i) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		char label[2] = { (char)('0' + i), 0 };
		bool clicked = ImGui::SmallButton(label);
		ImGui::PopStyleColor(3);
		if(clicked) m_channel_map[i] = !m_channel_map[i];
	}


	ImVec2 cursor = ImGui::GetCursorScreenPos();
	ImVec2 avail = ImGui::GetContentRegionAvail();

	SDL_Rect r = {
		(int)cursor.x,
		(int)cursor.y,
		(int)avail.x,
		(int)avail.y
	};

	SDL_SetRenderClipRect(rend, &r);
	if(m_type == Type::None) {

	} else if(m_type == Type::Spectogram) {

	} else if(m_type == Type::Waterfall) {
	
	} else if(m_type == Type::Waveform) {
		draw_waveform(streams, rend, r);
	}

	SDL_SetRenderClipRect(rend, nullptr);
}


void Widget::draw_waveform(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::SliderInt("Samples", &m_waveform.count, 100, 65536, "%d", ImGuiSliderFlags_Logarithmic);
	
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_waveform.agc);

	int midy = r.y + r.h / 2;
	float scale = 1.0;

	if(m_waveform.agc && m_waveform.peak > 0.0f) {
		scale = 1.0 / m_waveform.peak;
	}

	SDL_SetRenderDrawColor(rend, 100, 100, 100, 255);
	SDL_RenderLine(rend, r.x, midy, r.x + r.w, midy);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	int n = m_waveform.count;
	SDL_FPoint p_min[r.w];
	SDL_FPoint p_max[r.w];

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;

		for(int x=0; x<r.w; x++) {
			int idx_start = ((x+0) * n) / r.w;
			int idx_end   = ((x+1) * n) / r.w;
			float vmin, vmax;
			for(int idx=idx_start; idx<idx_end; idx++) {
				float v = streams.get(ch, idx);
				m_waveform.peak = (v > m_waveform.peak) ? v : m_waveform.peak;
				vmin = (idx == idx_start || v < vmin) ? v : vmin;
				vmax = (idx == idx_start || v > vmax) ? v : vmax;
			}
			p_min[x].x = (float)(r.x + x);
			p_min[x].y = (float)(midy - (int)(vmin * scale * (r.h / 2)));
			p_max[x].x = (float)(r.x + x);
			p_max[x].y = (float)(midy - (int)(vmax * scale * (r.h / 2)));
		}
		ImVec4 col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
		SDL_RenderLines(rend, p_min, r.w);
		SDL_RenderLines(rend, p_max, r.w);
	}
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	m_waveform.peak *= 0.95f;
}

