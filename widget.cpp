
#include <string>
#include <math.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"


static ImVec4 channel_color(int channel)
{
	static const ImVec4 color[] = {
		{ 0.00, 0.99, 0.00, 1.00 },
		{ 0.70, 0.00, 1.00, 1.00 },
		{ 0.00, 0.59, 1.00, 1.00 },
		{ 0.86, 0.49, 0.00, 1.00 },
		{ 1.00, 0.27, 0.33, 1.00 },
		{ 0.56, 0.61, 0.00, 1.00 },
		{ 0.00, 0.70, 0.00, 1.00 },
		{ 1.00, 0.00, 0.84, 1.00 },
	};
	if(channel >= 0 && channel < 8) {
		return color[channel];
	} else {
		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}


Widget::Widget(Type type)
	: m_type(type)
	, m_channel_map{}
	, m_waveform{
		.count = 1024,
		.agc = true,
		.peak = 0.0f,
	},
	m_spectrum{
		.size = 0,
		.in{},
		.out{},
		.plan = nullptr,
	}
{
	for(int i=0; i<8; i++) {
		m_channel_map[i] = true;
	}
	configure_fft(2048, Window::Type::Hanning);
}


void Widget::configure_fft(size_t size, Window::Type window_type)
{
	if(m_spectrum.plan) {
		fftwf_destroy_plan(m_spectrum.plan);
		m_spectrum.plan = nullptr;
	}
	m_spectrum.size = size;
	m_spectrum.in.resize(size);
	m_spectrum.out.resize(size);
	m_spectrum.plan = fftwf_plan_r2r_1d((int)size, m_spectrum.in.data(), m_spectrum.out.data(), FFTW_R2HC, FFTW_MEASURE);
	m_spectrum.window.configure(window_type, size);
}


void Widget::draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
{
	// Type selection combo box
	static const char *k_type_str[] = {
		"none", "spectrum", "waterfall", "wave"
	};
	ImGui::SetNextItemWidth(100);
	ImGui::Combo("type", (int *)&m_type, k_type_str, IM_ARRAYSIZE(k_type_str));

	// Channel enable buttons
	for(size_t i=0; i<8; i++) {
		ImGui::SameLine();
		ImVec4 col = m_channel_map[i] ? channel_color(i) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		char label[2] = { (char)('0' + i), 0 };
		bool clicked = ImGui::SmallButton(label);
		ImGui::PopStyleColor(3);
		if(clicked) m_channel_map[i] = !m_channel_map[i];
	}


	ImVec2 cursor = ImGui::GetCursorScreenPos();
	ImVec2 avail = ImGui::GetContentRegionAvail();

	SDL_Rect r = {
		(int)cursor.x,
		(int)cursor.y,
		(int)avail.x,
		(int)avail.y
	};

	SDL_SetRenderClipRect(rend, &r);
	if(m_type == Type::None) {

	} else if(m_type == Type::Spectrum) {
		 draw_spectrum(streams, rend, r);
	} else if(m_type == Type::Waterfall) {
	
	} else if(m_type == Type::Waveform) {
		draw_waveform(streams, rend, r);
	}

	SDL_SetRenderClipRect(rend, nullptr);
}


void Widget::draw_waveform(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::SliderInt("samples", &m_waveform.count, 100, 1024 * 1024, "%d", ImGuiSliderFlags_Logarithmic);
	
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_waveform.agc);

	int midy = r.y + r.h / 2;
	float scale = 1.0;

	if(m_waveform.agc && m_waveform.peak > 0.0f) {
		scale = 1.0 / m_waveform.peak;
	}

	SDL_SetRenderDrawColor(rend, 100, 100, 100, 255);
	SDL_RenderLine(rend, r.x, midy, r.x + r.w, midy);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	int step = 2;
	int npoints = r.w / step;
	int nsamples = m_waveform.count;
	SDL_FPoint p_min[npoints];
	SDL_FPoint p_max[npoints];

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;

		for(int i=0; i<npoints; i++) {
			int idx_start = ((i+0) * nsamples) / npoints;
			int idx_end   = ((i+1) * nsamples) / npoints;
			float vmin, vmax;
			for(int idx=idx_start; idx<idx_end; idx++) {
				float v = streams.get(ch, idx);
				m_waveform.peak = (v > m_waveform.peak) ? v : m_waveform.peak;
				vmin = (idx == idx_start || v < vmin) ? v : vmin;
				vmax = (idx == idx_start || v > vmax) ? v : vmax;
			}
			p_min[i].x = (float)(r.x + i * step);
			p_min[i].y = (float)(midy - (int)(vmin * scale * (r.h / 2)));
			p_max[i].x = (float)(r.x + i * step);
			p_max[i].y = (float)(midy - (int)(vmax * scale * (r.h / 2)));
		}
		ImVec4 col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
		SDL_RenderLines(rend, p_min, npoints);
		SDL_RenderLines(rend, p_max, npoints);
	}
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	m_waveform.peak *= 0.9f;
}


void Widget::draw_spectrum(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	static const char *k_type_str[] = { "rect", "hanning", "hamming", "blackman", "gauss" };
	
	bool changed = ImGui::Combo("window", (int *)&m_spectrum.window_type, k_type_str, IM_ARRAYSIZE(k_type_str));
	if(changed) {
		configure_fft(m_spectrum.size, m_spectrum.window_type);
	}

	size_t npoints = m_spectrum.size / 2 + 1;
	SDL_FPoint p[npoints];
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;

		streams.get(ch, 0, &m_spectrum.in[0], m_spectrum.size);

		for(size_t i=0; i<m_spectrum.size; i++) {
			m_spectrum.in[i] *= m_spectrum.window.get_data(i);
		}

		fftwf_execute(m_spectrum.plan);

		for(size_t i=0; i<m_spectrum.size; i++) {
			float v;
			if(i == 0) {
				v = fabsf(m_spectrum.out[0]);
			} else if(i < m_spectrum.size / 2) {
				v = hypotf(fabsf(m_spectrum.out[i]), fabsf(m_spectrum.out[m_spectrum.size - i]));
			} else if(i == m_spectrum.size / 2) {
				v = fabsf(m_spectrum.out[m_spectrum.size / 2]);
			} else {
				v = 0.0f;
			}
			m_spectrum.out[i] = v;
		}

		for(size_t i=0; i<npoints; i++) {
			float v = 20.0f * log10f(m_spectrum.out[i] + 1e-6f);
			int h = (int)((v + 100.0f) * (r.h / 100.0f)) - 30;
			p[i].x = r.x + (i * r.w) / npoints;
			p[i].y = r.y + r.h - h;
		}
		ImVec4 col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
		SDL_RenderLines(rend, p, npoints);
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}
