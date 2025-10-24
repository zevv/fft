
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "flap.hpp"
		


Flap::Flap(Widget::Type type)
	: m_type(type)
	, m_channel_map{}
	, m_waveform()
{
	for(int i=0; i<8; i++) {
		m_channel_map[i] = true;
	}
}


Flap::~Flap()
{
}


void Flap::load(ConfigReader::Node *n)
{
	if(const char *type = n->read_str("type")) {
		m_type = Widget::string_to_type(type);
		int channel_map = 0;
		if(n->read("channel_map", channel_map)) {
			for(int i=0; i<8; i++) {
				m_channel_map[i] = (channel_map & (1 << i)) ? true : false;
			}
		}
		if(m_type == Widget::Type::Waveform) m_waveform.load(n->find("waveform"));
		if(m_type == Widget::Type::Spectrum) m_spectrum.load(n->find("spectrum"));
		if(m_type == Widget::Type::Waterfall) m_waterfall.load(n->find("waterfall"));
	}
}


void Flap::save(ConfigWriter &cw)
{
	cw.push("widget");

	int channel_map = 0;
	for(int i=0; i<8; i++) {
		if(m_channel_map[i]) channel_map |= (1 << i);
	}

	cw.write("type", Widget::type_to_string(m_type));
	cw.write("channel_map", channel_map);

	if(m_type == Widget::Type::Waveform) m_waveform.save(cw);
	if(m_type == Widget::Type::Spectrum) m_spectrum.save(cw);
	if(m_type == Widget::Type::Waterfall) m_waterfall.save(cw);

	cw.pop();

}


Flap *Flap::copy(void)
{
	Flap *w = new Flap(m_type);
	for(int i=0; i<8; i++) {
		w->m_channel_map[i] = m_channel_map[i];
	}
	m_waveform.copy_to(w->m_waveform);
	m_spectrum.copy_to(w->m_spectrum);
	m_waterfall.copy_to(w->m_waterfall);
	return w;
}



void Flap::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
{
	m_has_focus = ImGui::IsWindowFocused();

	ImGui::SetNextItemWidth(100);
	ImGui::Combo("type", (int *)&m_type, 
			Widget::type_names(), Widget::type_count());

	// Channel enable buttons
	for(size_t i=0; i<8; i++) {
		ImGui::SameLine();
		SDL_Color c = Widget::channel_color(i);
		ImVec4 col = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		if(channel_enabled(i)) col = ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		char label[2] = { (char)('1' + i), 0 };
		ImGuiKey key = (ImGuiKey)(ImGuiKey_1 + i);
		if(ImGui::SmallButton(label) || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(key))) {
			// with shift: solo, otherwise toggle
			if(ImGui::GetIO().KeyShift) for(int j=0; j<8; j++) m_channel_map[j] = 0;
			m_channel_map[i] = !m_channel_map[i];
		}
		ImGui::PopStyleColor(3);
	}

	ImGui::SameLine();
	ImGui::Checkbox("L##lock view", &m_lock_view);

	if(ImGui::IsWindowFocused()) {
		if(ImGui::IsKeyPressed(ImGuiKey_0)) {
			for(int i=0; i<8; i++) m_channel_map[i] ^= 1;
		}
		if(ImGui::IsKeyPressed(ImGuiKey_F1)) m_type = Widget::Type::Waveform;
		if(ImGui::IsKeyPressed(ImGuiKey_F2)) m_type = Widget::Type::Spectrum;
		if(ImGui::IsKeyPressed(ImGuiKey_F3)) m_type = Widget::Type::Waterfall;
		if(ImGui::IsKeyPressed(ImGuiKey_L)) m_lock_view = !m_lock_view;
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

	if(m_type == Widget::Type::Waveform) m_waveform.draw(*this, view, streams, rend, r);
	if(m_type == Widget::Type::Spectrum) m_spectrum.draw(*this, view, streams, rend, r);
	if(m_type == Widget::Type::Waterfall) m_waterfall.draw(*this, view, streams, rend, r);
	

	SDL_SetRenderClipRect(rend, nullptr);
}


