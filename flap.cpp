
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>



Flap::Flap(Widget::Type type)
	: m_type(type)
	, m_waveform()
{
}


Flap::~Flap()
{
}


void Flap::load(ConfigReader::Node *n)
{
	if(const char *type = n->read_str("type")) {
		if(m_type == Widget::Type::Waveform) m_waveform.load(n->find("waveform"));
		if(m_type == Widget::Type::Spectrum) m_spectrum.load(n->find("spectrum"));
		if(m_type == Widget::Type::Waterfall) m_waterfall.load(n->find("waterfall"));
	}
}


void Flap::save(ConfigWriter &cw)
{
	cw.push("widget");
	if(m_type == Widget::Type::Waveform) m_waveform.save(cw);
	if(m_type == Widget::Type::Spectrum) m_spectrum.save(cw);
	if(m_type == Widget::Type::Waterfall) m_waterfall.save(cw);
	cw.pop();

}


Flap *Flap::copy(void)
{
	//m_waveform.copy_to(w->m_waveform);
	//m_spectrum.copy_to(w->m_spectrum);
	//m_waterfall.copy_to(w->m_waterfall);
	//return w;
	return nullptr;
}



void Flap::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
{

	if(ImGui::IsWindowFocused()) {
		if(ImGui::IsKeyPressed(ImGuiKey_F1)) m_type = Widget::Type::Waveform;
		if(ImGui::IsKeyPressed(ImGuiKey_F2)) m_type = Widget::Type::Spectrum;
		if(ImGui::IsKeyPressed(ImGuiKey_F3)) m_type = Widget::Type::Waterfall;
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


