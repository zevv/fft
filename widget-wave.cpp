
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "widgetinfo.hpp"
#include "widget-wave.hpp"


WidgetWaveform::WidgetWaveform(WidgetInfo &info)
	: Widget(info)
{
}


WidgetWaveform::~WidgetWaveform()
{
}


void WidgetWaveform::do_load(ConfigReader::Node *node)
{
	auto *wnode = node->find("waveform");
	wnode->read("agc", m_agc);
}


void WidgetWaveform::do_save(ConfigWriter &cw)
{
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.pop();
}


void WidgetWaveform::do_copy(Widget *w)
{
	WidgetWaveform *ww = dynamic_cast<WidgetWaveform*>(w);
	ww->m_agc = m_agc;
	ww->m_peak = m_peak;
}


void WidgetWaveform::do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
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
	
		auto pos = ImGui::GetIO().MousePos;

		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("t=%.4gs", m_view.time.cursor);

		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_t(ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_t(ImGui::GetIO().MouseDelta.y);
		}

		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.time.from = 0;
			m_view.time.to   = frames_avail / streams.sample_rate();
		}

	   if(ImGui::IsMouseInRect(r)) {

		   if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			   streams.player.seek(m_view.x_to_t(pos.x, r));
		   }

		   if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			   m_view.time.cursor += m_view.dx_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
		   } else {
			   m_view.time.cursor = m_view.x_to_t(pos.x, r);
		   }
	   }
	}

	Sample scale = k_sample_max;
	if(m_agc && m_peak > 0.0f) {
		scale = m_peak / 1.0;
	}
	m_peak *= 0.9f;

	float idx_from = m_view.x_to_t(r.x,       r) * streams.sample_rate();
	float idx_to   = m_view.x_to_t(r.x + r.w, r) * streams.sample_rate();
	float step = (idx_to - idx_from) / r.w;

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(auto ch : m_channel_map.enabled_channels()) {

		SDL_Color col = m_channel_map.ch_color(ch);
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


	// time grid
	grid_time(rend, r, m_view.x_to_t(r.x, r), m_view.x_to_t(r.x + r.w, r));
	
	// value grid
	grid_vertical(rend, r, -scale / k_sample_max, +scale / k_sample_max);

	// cursor
	int cx = m_view.t_to_x(m_view.time.cursor, r);
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_RenderLine(rend, cx - 1, r.y, cx - 1, r.y + r.h);
	SDL_RenderLine(rend, cx + 1, r.y, cx + 1, r.y + r.h);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	SDL_SetRenderDrawColor(rend, 192, 192, 192, 255);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);
	
	// play position
	SDL_SetRenderDrawColor(rend, 0, 96, 96, 255);
	int px = m_view.t_to_x(m_view.time.playpos, r);
	SDL_Vertex vert[3];
	vert[0].position = { (float)(px - 5), (float)(r.y) };
	vert[1].position = { (float)(px + 5), (float)(r.y) };
	vert[2].position = { (float)(px	), (float)(r.y + 8) };
	vert[0].color = vert[1].color = vert[2].color = { 0, 64, 128, 255 };
	SDL_RenderGeometry(rend, nullptr, vert, 3, nullptr, 0);
	vert[0].position = { (float)(px - 4), (float)(r.y + r.h - 1) };
	vert[1].position = { (float)(px + 4), (float)(r.y + r.h - 1) };
	vert[2].position = { (float)(px	), (float)(r.y + r.h - 9) };
	SDL_RenderGeometry(rend, nullptr, vert, 3, nullptr, 0);
	SDL_RenderLine(rend, px, r.y, px, r.y + r.h);


	// window
	Window w = Window(m_view.window.window_type, m_view.window.size, m_view.window.window_beta);
	SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
	double w_idx_from = (m_view.time.from - m_view.time.playpos) * streams.sample_rate() + m_view.window.size * 0.5;
	double w_idx_to   = (m_view.time.to   - m_view.time.playpos) * streams.sample_rate() + m_view.window.size * 0.5;
	graph(rend, r, w.data().data(), w.size(), 1, w_idx_from, w_idx_to, 0.0f, +1.0f);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetWaveform,
	.name = "waveform",
	.description = "Waveform display",
	.hotkey = ImGuiKey_F1,
	.flags = WidgetInfo::Flags::ShowChannelMap | WidgetInfo::Flags::ShowLock,
);

