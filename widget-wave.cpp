
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"

Widget::Waveform::Waveform()
	: m_agc(true)
	, m_peak(0.0f)
{
}


void Widget::Waveform::load(ConfigReader::Node *node)
{
	if(!node) return;
	node->read("agc", m_agc);
}


void Widget::Waveform::save(ConfigWriter &cw)
{
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.pop();
}


void Widget::Waveform::copy_to(Waveform &w)
{
	w.m_agc = m_agc;
	w.m_peak = m_peak;
}


void Widget::Waveform::draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_agc);
	ImGui::SameLine();
	ImGui::Text("| %d..%d @%d", (int)view.wave_from, (int)view.wave_to, (int)view.cursor);
	ImGui::SameLine();
	ImGui::Text("| peak: %.2f", m_peak);
		   
	if(ImGui::IsWindowFocused()) {
		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) view.cursor = view.x_to_idx(pos.x, r);
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			view.pan(-view.dx_to_didx(ImGui::GetIO().MouseDelta.x, r));
			view.zoom(ImGui::GetIO().MouseDelta.y / 100.0);
		}
	}

	float scale = 1.0;
	if(m_agc && m_peak > 0.0f) {
		scale = m_peak / 0.9;
	}
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	for(int ch=0; ch<8; ch++) {
		if(!widget.channel_enabled(ch)) continue;
		size_t stride;
		float *data = streams.peek(ch, 0, stride);
		ImVec4 col = widget.channel_color(ch);

		int idx_from = view.x_to_idx(r.x, r) - 1;
		int idx_to   = view.x_to_idx(r.x + r.w, r) + 1;

		float peak = widget.graph(view, rend, r, col, data, stride,
				idx_from, idx_to,
				-scale, +scale);
		if(peak > m_peak) {
			m_peak = peak;
		}
	}

	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = view.idx_to_x(view.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);

	// zero Y
	SDL_SetRenderDrawColor(rend, 100, 100, 100, 255);
	SDL_RenderLine(rend, r.x, r.y + r.h / 2, r.x + r.w, r.y + r.h / 2);

	// zero X
	float x = view.idx_to_x(0.0f, r);
	if(x >= r.x && x <= r.x + r.w) {
		SDL_SetRenderDrawColor(rend, 0, 255, 255, 128);
		SDL_RenderLine(rend, x, r.y, x, r.y + r.h);
	}
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	// window
	if(0 && view.window) {
		size_t wsize = view.window->size();
		const float *wdata = view.window->data();
		SDL_FPoint p[66];
		p[0].x = view.idx_to_x(view.cursor, r);
		p[0].y = r.y + r.h - 1;
		p[65].x = view.idx_to_x(view.cursor - wsize, r);
		p[65].y = r.y + r.h - 1;
		for(int i=0; i<64; i++) {
			int n = wsize * i / 64;
			p[i+1].x = view.idx_to_x(view.cursor - wsize * i / 63.0, r);
			p[i+1].y = r.y + (r.h-1) * (1.0f - wdata[n]);
		}

		SDL_SetRenderDrawColor(rend, 255, 255, 255, 128);
		SDL_RenderLines(rend, p, 66);
	}

	m_peak *= 0.9f;
}
