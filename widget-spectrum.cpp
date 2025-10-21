
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"
		


Widget::Spectrum::Spectrum()
{
	configure_fft(m_size, m_window_type);
}


Widget::Spectrum::~Spectrum()
{
	if(m_plan) {
		FFTW_DESTROY_PLAN(m_plan);
		m_plan = nullptr;
	}
}

void Widget::Spectrum::load(ConfigReader::Node *node)
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


void Widget::Spectrum::save(ConfigWriter &cw)
{
	cw.push("spectrum");
	cw.write("fft_size", (int)m_size);
	cw.write("window_type", Window::type_to_str(m_window_type));
	cw.write("window_beta", m_window_beta);
	cw.write("freq_from", m_freq_from);
	cw.write("freq_to", m_freq_to);
	cw.pop();
}


void Widget::Spectrum::copy_to(Spectrum &w)
{
	w.m_size = m_size;
	w.m_window_type = m_window_type;
	w.m_window_beta = m_window_beta;
	w.configure_fft(w.m_size, w.m_window_type);
};


void Widget::Spectrum::draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
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
	
	if(m_window_type == Window::Type::Gauss || 
	   m_window_type == Window::Type::Kaiser) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		bool changed = ImGui::SliderFloat("beta", &m_window_beta, 
				0.0f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if(changed) {
			m_window.configure(m_window_type, m_size, m_window_beta);
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

	if(ImGui::IsWindowFocused()) {
		view.window = &m_window;
	}
	
	if(update) {
		configure_fft(m_size, m_window_type);
	}

	
	// grid
	
	float db_range = -120.0;

	SDL_SetRenderDrawColor(rend, 64, 64, 64, 255);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	for(float dB=db_range; dB<=0.0f; dB+=10.0f) {
		int y = r.y - dB * r.h / -db_range;
		SDL_RenderLine(rend, r.x, y, r.x + r.w, y);
		char buf[32];
		snprintf(buf, sizeof(buf), "%+.0f", dB);
		dl->AddText(ImVec2((float)r.x + 4, y), ImColor(64, 64, 64, 255), buf);
	}
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	// spectograms

	for(int ch=0; ch<8; ch++) {
		if(!widget.channel_enabled(ch)) continue;

		size_t stride = 0;
		Sample *data = streams.peek(ch, -(view.srate * view.cursor) + m_size, stride);

		const Sample *w = m_window.data();
		for(int i=0; i<m_size; i++) {
			m_in[i] = data[stride * i] * w[i];
		}

		FFTW_EXECUTE(m_plan);
		float scale = 2.0f / m_size / k_sample_max;

		for(int i=0; i<m_size; i++) {
			SampleFFTW v = 0.0;
			if(i == 0) {
				v = m_out[0] * scale / 2;
			} else if(i < m_size / 2) {
				v = hypot(m_out[i], m_out[m_size - i]) * scale;
			} else if(i == m_size / 2) {
				v = fabs(m_out[m_size / 2]) * scale / 2;
			} 
			m_out_graph[i] = (v >= 1e-20f) ? 20.0f * log10f(v) : db_range;
		}

		size_t npoints = m_size / 2 + 1;
		ImVec4 col = widget.channel_color(ch);
		widget.graph(rend, r, col, m_out_graph.data(), 1,
				m_freq_from * npoints, m_freq_to * npoints,
				0, npoints,
				db_range, 0.0);
	}
	
	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = freq_to_x(m_freq_cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);


	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


void Widget::Spectrum::configure_fft(int size, Window::Type window_type)
{
	if(m_plan) {
		FFTW_DESTROY_PLAN(m_plan);
		m_plan = nullptr;
	}
	m_size = size;
	m_in.resize(size);
	m_out.resize(size);
	m_out_graph.resize(size);

	m_plan = FFTW_PLAN_R2R_1D(size, m_in.data(), m_out.data(), FFTW_R2HC, FFTW_ESTIMATE);

	m_window.configure(window_type, size);
}

