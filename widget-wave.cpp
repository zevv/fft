
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget-wave.hpp"


WidgetWaveform::WidgetWaveform()
	: Widget(Widget::Type::Waveform)
{
}


WidgetWaveform::~WidgetWaveform()
{
}


void WidgetWaveform::load(ConfigReader::Node *node)
{
	if(!node) return;
	Widget::load(node);
	if(auto *wnode = node->find("waveform")) {
		wnode->read("agc", m_agc);
	}
}


void WidgetWaveform::save(ConfigWriter &cw)
{
	Widget::save(cw);
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.pop();
}


Widget *WidgetWaveform::do_copy()
{
	WidgetWaveform *w = new WidgetWaveform();
	w->m_agc = m_agc;
	w->m_peak = m_peak;
	return w;
}


void WidgetWaveform::do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_agc);

	if(ImGui::IsWindowFocused()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("t=%.4gs", m_view.t_cursor);

		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_t(ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_t(ImGui::GetIO().MouseDelta.y);
		}

		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			size_t used;
			size_t stride;
			streams.peek(0, &stride, &used);
			m_view.t_from = 0;
			m_view.t_to   = used / view.srate;
		}

		auto pos = ImGui::GetIO().MousePos;
		if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			m_view.t_cursor += m_view.dx_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
		} else {
			m_view.t_cursor = m_view.x_to_t(pos.x, r);
		}
	}

	Time dt_min = 16.0 / view.srate;
	if(m_view.t_to - m_view.t_from < dt_min) {
		m_view.t_from = m_view.t_to - dt_min;
	}

	Sample scale = k_sample_max;
	if(m_agc && m_peak > 0.0f) {
		scale = m_peak / 0.9;
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;
		size_t stride;
		size_t avail;
		Sample *data = streams.peek(ch, &stride, &avail);

		float idx_from = m_view.x_to_t(r.x,       r) * view.srate;
		float idx_to   = m_view.x_to_t(r.x + r.w, r) * view.srate;

		SDL_Color col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);

		float peak = graph(rend, r,
				data, avail, stride,
				idx_from, idx_to,
				-scale, +scale);

		if(peak > m_peak) {
			m_peak = peak;
		}
	}

	grid_time(rend, r, m_view.x_to_t(r.x, r), m_view.x_to_t(r.x + r.w, r));

	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.t_to_x(m_view.t_cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);

	// zero Y
	SDL_SetRenderDrawColor(rend, 100, 100, 100, 255);
	SDL_RenderLine(rend, r.x, r.y + r.h / 2, r.x + r.w, r.y + r.h / 2);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	// window
	#if 0
	if(0 && view.window) {
		size_t wsize = view.window->size();
		const Sample *wdata = view.window->data();
		SDL_FPoint p[66];
		p[0].x = t_to_x(view.cursor, r);
		p[0].y = r.y + r.h - 1;
		p[65].x = t_to_x(view.cursor - wsize, r);
		p[65].y = r.y + r.h - 1;
		for(int i=0; i<64; i++) {
			int n = wsize * i / 64;
			p[i+1].x = t_to_x(view.cursor - wsize * i / 63.0, r);
			p[i+1].y = r.y + (r.h-1) * (1.0f - wdata[n]);
		}

		SDL_SetRenderDrawColor(rend, 255, 255, 255, 128);
		SDL_RenderLines(rend, p, 66);
	}
#endif

	m_peak *= 0.9f;
}
