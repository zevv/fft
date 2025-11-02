
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetinfo.hpp"
#include "widget-spectrum.hpp"


WidgetSpectrum::WidgetSpectrum()
	: Widget("spectrum")
{
}


WidgetSpectrum::~WidgetSpectrum()
{
}


void WidgetSpectrum::do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SetNextItemWidth(100);
	ImGui::SameLine();
	ImGui::SliderInt("##fft size", (int *)&m_view.fft.size, 
				16, 32768, "%d", ImGuiSliderFlags_Logarithmic);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::Combo("##window", (int *)&m_view.fft.window_type, 
			Window::type_names(), Window::type_count());

	if(m_view.fft.window_type == Window::Type::Gauss || 
	   m_view.fft.window_type == Window::Type::Kaiser) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		float beta = m_view.fft.window_beta;
		ImGui::SliderFloat("beta", &beta,
				0.0f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		m_view.fft.window_beta = beta;
	}
	
	if(has_focus()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("f=%.6gHz amp=%.2fdB", m_view.freq.cursor * m_view.srate * 0.5, m_amp_cursor);

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

	m_fft.configure(m_view.fft.size, m_view.fft.window_type, m_view.fft.window_beta);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	int8_t db_range = -120.0;
	grid_vertical(rend, r, db_range, 0);

	for(int ch : m_channel_map.enabled_channels()) {
		size_t stride = 0;
		size_t avail = 0;
		Sample *data = streams.peek(&stride, &avail);
		int idx = ((int)(m_view.srate * m_view.time.cursor)) * stride + ch;

		if(idx < 0) continue;
		if(idx >= (int)(avail * stride)) continue;


		auto out_graph = m_fft.run(&data[idx], stride);

		size_t npoints = m_view.fft.size / 2 + 1;
		SDL_Color col = m_channel_map.ch_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);
		graph(rend, r,
				out_graph.data(), out_graph.size(), 1,
				m_view.freq.from * npoints, m_view.freq.to * npoints,
				db_range, (int8_t)0);
	}
	
	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.freq_to_x(m_view.freq.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);


	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetSpectrum,
	.name = "spectrum",
	.description = "FFT spectrum graph",
	.hotkey = ImGuiKey_F2,
);

