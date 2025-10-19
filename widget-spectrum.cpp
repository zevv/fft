
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"
		


Widget::Spectrum::Spectrum()
	: m_size(2048)
	, m_window_type(Window::Type::Hanning)
	, m_window_beta(5.0f)
	, m_plan(nullptr)
{
	configure_fft(m_size, m_window_type);
}


Widget::Spectrum::~Spectrum()
{
	if(m_plan) {
		fftwf_destroy_plan(m_plan);
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
	configure_fft(m_size, m_window_type);
}


void Widget::Spectrum::save(ConfigWriter &cw)
{
	cw.push("spectrum");
	cw.write("fft_size", (int)m_size);
	cw.write("window_type", Window::type_to_str(m_window_type));
	cw.write("window_beta", m_window_beta);
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
	
	if(ImGui::IsWindowFocused()) {
		//ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		update |= ImGui::SliderInt("fft size", (int *)&m_size, 
					16, 32768, "%d", ImGuiSliderFlags_Logarithmic);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		update |= ImGui::Combo("window", (int *)&m_window_type, 
				Window::type_names(), Window::type_count());
	}

	if(ImGui::IsWindowFocused()) {
		view.window = &m_window;
	}
	
	if(update) {
		configure_fft(m_size, m_window_type);
	}

	if(m_window_type == Window::Type::Gauss) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		bool changed = ImGui::SliderFloat("beta", &m_window_beta, 
				0.0f, 40.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if(changed) {
			m_window.configure(m_window_type, 
					                    m_size, m_window_beta);
		}
	}

	size_t npoints = m_size / 2 + 1;
	
	// grid

	SDL_SetRenderDrawColor(rend, 64, 64, 64, 255);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	for(float dB=-100.0f; dB<=0.0f; dB+=10.0f) {
		int y = view.db_to_y(dB, r);
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
		float *data = streams.peek(ch, -view.cursor + m_size, stride);

		const float *w = m_window.data();
		float gain = m_window.gain();
		for(int i=0; i<m_size; i++) {
			m_in[i] = data[stride * i] * w[i] * gain;
		}

		fftwf_execute(m_plan);

		float scale = 2.0f / (float)m_size * m_window.gain();
		for(int i=0; i<m_size; i++) {
			float v;
			     if(i == 0) v = fabsf(m_out[0]);
			else if(i < m_size / 2) v = hypotf(fabsf(m_out[i]), fabsf(m_out[m_size - i]));
			else if(i == m_size / 2) v = fabsf(m_out[m_size / 2]);
			else v = 0.0f;
			m_out[i] = (v >= 1e-20f) ? 20.0f * log10f(v) : -100.0f;
		}

		ImVec4 col = widget.channel_color(ch);
		float p = widget.graph(rend, r, col, m_out.data(), 1,
				0, npoints,
				0, -100);
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


void Widget::Spectrum::configure_fft(int size, Window::Type window_type)
{
	if(m_plan) {
		fftwf_destroy_plan(m_plan);
		m_plan = nullptr;
	}
	m_size = size;
	m_in.resize(size);
	m_out.resize(size);

	int sizes[] = {size};
	fftwf_r2r_kind kinds[] = { FFTW_R2HC };
	m_plan = fftwf_plan_many_r2r(
			1, sizes, 1, 
			m_in.data(), NULL, 1, 0, 
			m_out.data(), NULL, 1, 0, 
			kinds, FFTW_ESTIMATE);

	m_window.configure(window_type, size);
}

