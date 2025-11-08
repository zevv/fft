
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

		   if(ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			   streams.player.seek(m_view.x_to_t(pos.x, r));
		   }

		   if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			   m_view.time.cursor += m_view.dx_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
		   } else {
			   m_view.time.cursor = m_view.x_to_t(pos.x, r);
		   }

		   // if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		   //     m_view.time.sel_from = m_view.x_to_t(pos.x, r);
		   //     m_view.time.sel_to   = m_view.x_to_t(pos.x, r);
		   // }

		   // if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		   //     if(ImGui::GetIO().MouseDelta.x < 10) {
		   //  	   m_view.time.sel_to = m_view.x_to_t(pos.x, r);
		   //     }
		   // }

		   // if(m_view.time.sel_from != m_view.time.sel_to) {
		   //     if(m_view.time.playpos < m_view.time.sel_from || m_view.time.playpos > m_view.time.sel_to) {
		   //  	   streams.player.seek(m_view.time.sel_from);
		   //  	   printf("Seek to sel_from %.4f\n", m_view.time.sel_from);
		   //  	   m_view.time.playpos = m_view.time.sel_from + 0.1; // bah
		   //     }
		   // }
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
					(Sample)-scale, (Sample)+scale);
		}

		m_peak = std::max(m_peak, peak);
	}
	
	// selection
	if(m_view.time.sel_from != m_view.time.sel_to) {
		float sx_from = m_view.t_to_x(m_view.time.sel_from, r);
		float sx_to   = m_view.t_to_x(m_view.time.sel_to,   r);
		SDL_SetRenderDrawColor(rend, 128, 128, 255, 64);
		SDL_FRect sr = { sx_from, (float)r.y, sx_to - sx_from, (float)r.h };
		SDL_RenderFillRect(rend, &sr);
	}

	// grids
	grid_time(rend, r, m_view.x_to_t(r.x, r), m_view.x_to_t(r.x + r.w, r));
	grid_vertical(rend, r, -scale / k_sample_max, +scale / k_sample_max);

	// window
	Window w = Window(m_view.window.window_type, m_view.window.size, m_view.window.window_beta);
	SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
	double w_idx_from = (m_view.time.from - m_view.time.playpos) * streams.sample_rate() + m_view.window.size * 0.5;
	double w_idx_to   = (m_view.time.to   - m_view.time.playpos) * streams.sample_rate() + m_view.window.size * 0.5;
	graph(rend, r, w.data().data(), w.size(), 1, w_idx_from, w_idx_to, 0.0f, +1.0f);

	// cursor
	int cx = m_view.t_to_x(m_view.time.cursor, r);
	hcursor(rend, r, cx, false);
	
	// play position
	int px = m_view.t_to_x(m_view.time.playpos, r);
	hcursor(rend, r, px, true);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetWaveform,
	.name = "waveform",
	.description = "Waveform display",
	.hotkey = ImGuiKey_F1,
	.flags = WidgetInfo::Flags::ShowChannelMap | WidgetInfo::Flags::ShowLock,
);

