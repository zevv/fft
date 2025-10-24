
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "flap.hpp"
#include "widget-spectrum.hpp"


Spectrum::Spectrum()
{
	configure_fft(m_size, m_window_type);
}


Spectrum::~Spectrum()
{
}


void Spectrum::load(ConfigReader::Node *node)
{
	if(!node) return;
	if(const char *window_type = node->read_str("window_type")) {
		m_window_type = Window::str_to_type(window_type);
	}
	node->read("window_beta", m_window_beta);
	node->read("fft_size", m_size);
	node->read("freq_from", m_freq_from);
	node->read("freq_to", m_freq_to);
	configure_fft(m_size, m_window_type);
}


void Spectrum::save(ConfigWriter &cw)
{
	cw.push("spectrum");
	cw.write("fft_size", (int)m_size);
	cw.write("window_type", Window::type_to_str(m_window_type));
	cw.write("window_beta", m_window_beta);
	cw.write("freq_from", m_freq_from);
	cw.write("freq_to", m_freq_to);
	cw.pop();
}


void Spectrum::copy_to(Spectrum &w)
{
	w.m_size = m_size;
	w.m_window_type = m_window_type;
	w.m_window_beta = m_window_beta;
	w.configure_fft(w.m_size, w.m_window_type);
};


void Spectrum::draw(Flap &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	bool update = false;
	
	ImGui::SetNextItemWidth(100);
	ImGui::SameLine();
	update |= ImGui::SliderInt("##fft size", (int *)&m_size, 
				16, 32768, "%d", ImGuiSliderFlags_Logarithmic);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	update |= ImGui::Combo("##window", (int *)&m_window_type, 
			Window::type_names(), Window::type_count());
	
	if(update) {
		configure_fft(m_size, m_window_type);
	}

	if(m_window_type == Window::Type::Gauss || 
	   m_window_type == Window::Type::Kaiser) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		bool changed = ImGui::SliderFloat("beta", &m_window_beta, 
				0.0f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if(changed) {
			m_fft.set_window(m_window_type, m_size, m_window_beta);
		}
	}

	if(widget.has_focus()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("f=%.6gHz amp=%.2fdB", m_freq_cursor * view.srate * 0.5, m_amp_cursor);

		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_freq_cursor += dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			} else {
				m_freq_cursor = x_to_freq(pos.x, r);
			}
			m_amp_cursor = (r.y - pos.y) * 100.0f / r.h;
		}
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			pan(-dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r));
			zoom(ImGui::GetIO().MouseDelta.y / 100.0f);
		}
	
		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_freq_from = 0.0f;
			m_freq_to = 1.0;
		}
	}

	if(m_freq_from < 0.0f) m_freq_from = 0.0f;
	if(m_freq_to > 1.0f) m_freq_to = 1.0f;

	

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	float db_range = -120.0;
	widget.grid_vertical(rend, r, db_range, 0);


	// spectograms
		
	for(int ch=0; ch<8; ch++) {
		if(!widget.channel_enabled(ch)) continue;

		size_t stride = 0;
		size_t avail = 0;
		Sample *data = streams.peek(ch, &stride, &avail);
		//int idx = ((int)(view.srate * view.cursor) - m_window.size() / 2) * stride;; TODO
		int idx = ((int)(view.srate * view.cursor)) * stride;;

		for(int i=0; i<m_size; i++) {
			Sample v = 0;
			if(idx >= 0 && idx < (int)(avail * stride)) {
				v = data[idx];
			}
			m_in[i] = v;
			idx += stride;;
		}

		auto m_out_graph = m_fft.run(m_in);
		// float scale = 2.0f / m_size / k_sample_max; TODO

		size_t npoints = m_size / 2 + 1;
		SDL_Color col = widget.channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);
		widget.graph(rend, r,
				m_out_graph.data(), m_out_graph.size(), 1,
				m_freq_from * npoints, m_freq_to * npoints,
				db_range, 0.0);
	}
	
	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = freq_to_x(m_freq_cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);


	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


void Spectrum::configure_fft(int size, Window::Type window_type)
{
	m_size = size;
	m_in.resize(size);
	m_out_graph.resize(size);

	m_fft.set_size(size);
	m_fft.set_window(window_type, size, m_window_beta);

}
