
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"
		


void Widget::draw_spectrum(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	bool update = false;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	update |= ImGui::SliderInt("fft size", (int *)&m_spectrum.size, 
				16, 32768, "%d", ImGuiSliderFlags_Logarithmic);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	update |= ImGui::Combo("window", (int *)&m_spectrum.window_type, 
			Window::type_names(), Window::type_count());

	if(ImGui::IsWindowFocused()) {
		view.window = &m_spectrum.window;
	}
	
	if(update) {
		configure_fft(m_spectrum.size, m_spectrum.window_type);
	}

	if(m_spectrum.window_type == Window::Type::Gauss) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		bool changed = ImGui::SliderFloat("beta", &m_spectrum.window_beta, 
				0.0f, 40.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if(changed) {
			m_spectrum.window.configure(m_spectrum.window_type, 
					                    m_spectrum.size, m_spectrum.window_beta);
		}
	}

	size_t npoints = m_spectrum.size / 2 + 1;
	SDL_FPoint p[npoints];
	
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
		if(!m_channel_map[ch]) continue;

		size_t stride = 0;
		float *data = streams.peek(ch, -view.cursor + m_spectrum.size, stride);

		const float *w = m_spectrum.window.data();
		float gain = m_spectrum.window.gain();
		for(int i=0; i<m_spectrum.size; i++) {
			m_spectrum.in[i] = data[stride * i] * w[i] * gain;
		}

		fftwf_execute(m_spectrum.plan);

		float scale = 2.0f / (float)m_spectrum.size * m_spectrum.window.gain();
		for(int i=0; i<m_spectrum.size; i++) {
			float v;
			     if(i == 0) v = fabsf(m_spectrum.out[0]);
			else if(i < m_spectrum.size / 2) v = hypotf(fabsf(m_spectrum.out[i]), fabsf(m_spectrum.out[m_spectrum.size - i]));
			else if(i == m_spectrum.size / 2) v = fabsf(m_spectrum.out[m_spectrum.size / 2]);
			else v = 0.0f;
			m_spectrum.out[i] = (v >= 1e-20f) ? 20.0f * log10f(v) : -100.0f;
		}

		ImVec4 col = channel_color(ch);
		float p = draw_graph(view, rend, r, col, m_spectrum.out.data(), 1,
				0, npoints,
				0, -100);
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


