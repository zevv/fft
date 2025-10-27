
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget-spectrum.hpp"


WidgetSpectrum::WidgetSpectrum()
	: Widget(Widget::Type::Spectrum)
{
}


WidgetSpectrum::~WidgetSpectrum()
{
}


void WidgetSpectrum::load(ConfigReader::Node *node)
{
	if(!node) return;
	Widget::load(node);
	if(auto *wnode = node->find("spectrum")) {
		if(const char *window_type = wnode->read_str("window_type")) {
			m_view.window_type = Window::str_to_type(window_type);
		}
		wnode->read("window_beta", m_view.window_beta);
		wnode->read("fft_size", m_view.fft_size);
	}
}


void WidgetSpectrum::save(ConfigWriter &cw)
{
	Widget::save(cw);
	cw.push("spectrum");
	cw.write("fft_size", (int)m_view.fft_size);
	cw.write("window_type", Window::type_to_str(m_view.window_type));
	cw.write("window_beta", m_view.window_beta);
	cw.pop();
}


Widget *WidgetSpectrum::do_copy()
{
	auto *w = new WidgetSpectrum();
	return w;
};


void WidgetSpectrum::do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SetNextItemWidth(100);
	ImGui::SameLine();
	ImGui::SliderInt("##fft size", (int *)&m_view.fft_size, 
				16, 32768, "%d", ImGuiSliderFlags_Logarithmic);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::Combo("##window", (int *)&m_view.window_type, 
			Window::type_names(), Window::type_count());

	if(m_view.window_type == Window::Type::Gauss || 
	   m_view.window_type == Window::Type::Kaiser) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		float beta = m_view.window_beta;
		ImGui::SliderFloat("beta", &beta,
				0.0f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		m_view.window_beta = beta;
	}
	
	if(has_focus()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("f=%.6gHz amp=%.2fdB", m_view.freq_cursor * view.srate * 0.5, m_amp_cursor);

		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.freq_cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			} else {
				m_view.freq_cursor = m_view.x_to_freq(pos.x, r);
			}
			m_amp_cursor = (r.y - pos.y) * 100.0f / r.h;
		}
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_freq(-ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_freq(ImGui::GetIO().MouseDelta.y);
		}
	
		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.freq_from = 0.0f;
			m_view.freq_to = 1.0;
		}
	}

	if(m_view.freq_from < 0.0f) m_view.freq_from = 0.0f;
	if(m_view.freq_to > 1.0f) m_view.freq_to = 1.0f;
	
	m_fft.configure(m_view.fft_size, m_view.window_type, m_view.window_beta);
	m_in.resize(m_view.fft_size);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	float db_range = -120.0;
	grid_vertical(rend, r, db_range, 0);

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;

		size_t stride = 0;
		size_t avail = 0;
		Sample *data = streams.peek(&stride, &avail);
		int idx = ((int)(view.srate * m_view.t_cursor)) * stride + ch;

		for(int i=0; i<m_view.fft_size; i++) {
			Sample v = 0;
			if(idx >= 0 && idx < (int)(avail * stride)) {
				v = data[idx];
			}
			m_in[i] = v;
			idx += stride;;
		}

		auto out_graph = m_fft.run(m_in);

		size_t npoints = m_view.fft_size / 2 + 1;
		SDL_Color col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);
		graph(rend, r,
				out_graph.data(), out_graph.size(), 1,
				m_view.freq_from * npoints, m_view.freq_to * npoints,
				db_range, 0.0f);
	}
	
	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.freq_to_x(m_view.freq_cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);


	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


