
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetregistry.hpp"
#include "widget-spectrum.hpp"


WidgetSpectrum::WidgetSpectrum(Widget::Info &info)
	: Widget(info)
{
}


WidgetSpectrum::~WidgetSpectrum()
{
}


void WidgetSpectrum::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	if(ImGui::IsWindowFocused()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("f=%.6gHz amp=%.2fdB", m_view.freq.cursor * stream.sample_rate() * 0.5, m_amp_cursor);

		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.freq.cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			} else {
				m_view.freq.cursor = m_view.x_to_freq(pos.x, r);
			}
			m_amp_cursor = (r.y - pos.y) * 100.0f / r.h;
		}
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_freq(-ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_freq(ImGui::GetIO().MouseDelta.y);
		}
		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.freq.from = 0.0f;
			m_view.freq.to = 1.0;
		}
	}

	m_fft.configure(m_view.window.size, m_view.window.window_type, m_view.window.window_beta);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	int8_t db_range = -120.0;
	grid_vertical(rend, r, db_range, 0);

	for(int ch : m_channel_map.enabled_channels()) {
		size_t stride = 0;
		size_t avail = 0;
		Sample *data = stream.peek(&stride, &avail);
		int idx = ((int)(stream.sample_rate() * m_view.time.analysis - m_view.window.size * 0.5)) * stride + ch;

		if(idx < 0) continue;
		if(idx >= (int)(avail * stride)) continue;


		auto out_graph = m_fft.run(&data[idx], stride);

		size_t npoints = m_view.window.size / 2 + 1;
		SDL_Color col = m_channel_map.ch_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);
		graph(rend, r,
				out_graph.data(), out_graph.size(), 1,
				m_view.freq.from * npoints, m_view.freq.to * npoints,
				db_range, (int8_t)0);
	}
	
	// cursor
	int cx = m_view.freq_to_x(m_view.freq.cursor, r);
	hcursor(rend, r, cx, false);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetSpectrum,
	.name = "spectrum",
	.description = "FFT spectrum graph",
	.hotkey = ImGuiKey_F2,
	.flags = Widget::Info::Flags::ShowChannelMap | 
	         Widget::Info::Flags::ShowLock |
			 Widget::Info::Flags::ShowWindowSize |
			 Widget::Info::Flags::ShowWindowType,
);

