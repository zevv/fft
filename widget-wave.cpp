
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
	ImGui::ToggleButton("AGC", &m_agc);
	
	size_t frames_avail;
	size_t data_stride;
	Sample *data = streams.peek(&data_stride, &frames_avail);

	size_t wframes_avail;
	size_t wdata_stride;
	Wavecache::Range *wdata = streams.peek_wavecache(&wdata_stride, &wframes_avail);

	if(ImGui::IsWindowFocused()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("t=%.4gs", m_view.time.cursor);

		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_t(ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_t(ImGui::GetIO().MouseDelta.y);
		}

		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.time.from = 0;
			m_view.time.to   = frames_avail / view.srate;
		}

		auto pos = ImGui::GetIO().MousePos;
		if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			m_view.time.cursor += m_view.dx_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
		} else {
			m_view.time.cursor = m_view.x_to_t(pos.x, r);
		}
	}

	Sample scale = k_sample_max;
	if(m_agc && m_peak > 0.0f) {
		scale = m_peak / 0.9;
	}

	float idx_from = m_view.x_to_t(r.x,       r) * view.srate;
	float idx_to   = m_view.x_to_t(r.x + r.w, r) * view.srate;
	float step = (idx_to - idx_from) / r.w;

	ImGui::SameLine();
	ImGui::Text(" step: %.0f", step);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;

		SDL_Color col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);

		Sample peak;

		if(step < 256) {
			peak = graph(rend, r,
					data + ch, frames_avail, data_stride,
					idx_from, idx_to,
					(Sample)-scale, (Sample)+scale);
		} else {
			peak = graph(rend, r,
					&wdata[ch].min, &wdata[ch].max, 
					frames_avail / 256, wdata_stride * 2,
					idx_from / 256, idx_to / 256,
					(int8_t)(-scale / 256), (int8_t)(+scale / 256));
			peak *= 256;
		}

		m_peak = std::max(m_peak, peak);
	}

	grid_time(rend, r, m_view.x_to_t(r.x, r), m_view.x_to_t(r.x + r.w, r));

	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.t_to_x(m_view.time.cursor, r);
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
