
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"
#include "widget-wave.hpp"
#include "config.hpp"

Waveform::Waveform()
{
}


void Waveform::load(ConfigReader::Node *node)
{
	if(!node) return;
	node->read("agc", m_agc);
	node->read("t_from", m_t_from);
	node->read("t_to", m_t_to);
}


void Waveform::save(ConfigWriter &cw)
{
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.write("t_from", m_t_from);
	cw.write("t_to", m_t_to);
	cw.pop();
}


void Waveform::copy_to(Waveform &w)
{
	w.m_agc = m_agc;
	w.m_peak = m_peak;
	w.m_t_from = m_t_from;
	w.m_t_to = m_t_to;
	w.m_t_cursor = m_t_cursor;
}


void Waveform::draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_agc);

	if(ImGui::IsWindowFocused()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("t=%.4gs", m_t_cursor);

		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			pan(-dx_to_dt(ImGui::GetIO().MouseDelta.x, r));
			zoom(ImGui::GetIO().MouseDelta.y / 100.0);
		}

		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			size_t used;
			size_t stride;
			streams.peek(0, 0, stride, &used);
			m_t_from = -view.srate / used;
			m_t_to   = 0;
		}

		auto pos = ImGui::GetIO().MousePos;
		if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			view.cursor += dx_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
		} else {
			view.cursor = x_to_t(pos.x, r);
		}
		m_t_cursor = view.cursor;
	}
	
	if(m_t_to < m_t_from + 0.0001) { // TODO
		m_t_from = m_t_to - 0.0001;
	}

	Sample scale = k_sample_max;
	if(m_agc && m_peak > 0.0f) {
		scale = m_peak / 0.9;
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	for(int ch=0; ch<8; ch++) {
		if(!widget.channel_enabled(ch)) continue;
		size_t stride;
		size_t depth;
		Sample *data = streams.peek(ch, 0, stride, &depth);
		ImVec4 col = widget.channel_color(ch);

		float t_from = view.srate * x_to_t(r.x, r) - 1.0;
		float t_to   = view.srate * x_to_t(r.x + r.w, r);

		float peak = widget.graph(rend, r, col, data, stride,
				t_from, t_to, 
				-(float)depth, 0.0,
				-scale, +scale);

		if(peak > m_peak) {
			m_peak = peak;
		}
	}

	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = t_to_x(view.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);

	// zero Y
	SDL_SetRenderDrawColor(rend, 100, 100, 100, 255);
	SDL_RenderLine(rend, r.x, r.y + r.h / 2, r.x + r.w, r.y + r.h / 2);

	// zero X
	float x = t_to_x(0.0f, r);
	if(x >= r.x && x <= r.x + r.w) {
		SDL_SetRenderDrawColor(rend, 0, 255, 255, 128);
		SDL_RenderLine(rend, x, r.y, x, r.y + r.h);
	}
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	// window
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

	m_peak *= 0.9f;
}
