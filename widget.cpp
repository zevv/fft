
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

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
		.agc = true,
		.peak = 0.0f,
	},
	m_spectrum{
		.size = 0,
		.window_type = Window::Type::Gauss,
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
	m_spectrum.plan = fftwf_plan_r2r_1d((int)size, m_spectrum.in.data(), m_spectrum.out.data(), FFTW_R2HC, FFTW_ESTIMATE);
	m_spectrum.window.configure(window_type, size);
}


void Widget::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
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
		char label[2] = { (char)('1' + i), 0 };
		ImGuiKey key = (ImGuiKey)(ImGuiKey_1 + i);
		if(ImGui::SmallButton(label) ||
		   (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(key))) {
			m_channel_map[i] = !m_channel_map[i];
		}
		ImGui::PopStyleColor(3);
	}

	if(ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_0)) {
		for(int i=0; i<8; i++) m_channel_map[i] ^= 1;
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
		 draw_spectrum(view, streams, rend, r);
	} else if(m_type == Type::Waterfall) {
	
	} else if(m_type == Type::Waveform) {
		draw_waveform(view, streams, rend, r);
	}

	SDL_SetRenderClipRect(rend, nullptr);
}

void Widget::draw_waveform(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_waveform.agc);
		   
	if(ImGui::IsWindowFocused()) {
		view.cursor = view.x_to_idx(ImGui::GetIO().MousePos.x, r);
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			view.pan(view.dx_to_didx(ImGui::GetIO().MouseDelta.x, r));
			view.zoom(ImGui::GetIO().MouseDelta.y / 100.0);
		}
	}

	int midy = r.y + r.h / 2;
	float scale = 1.0;

	if(m_waveform.agc && m_waveform.peak > 0.0f) {
		scale = 1.0 / m_waveform.peak * 0.9;
	}

	SDL_SetRenderDrawColor(rend, 100, 100, 100, 255);
	SDL_RenderLine(rend, r.x, midy, r.x + r.w, midy);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	int npoints = r.w;
	int nsamples = view.to - view.from;
	SDL_FPoint p_min[npoints];
	SDL_FPoint p_max[npoints];
	SDL_FRect rect[npoints];

	ImGui::SameLine();
	ImGui::Text("| %d-%d %d", (int)view.from, (int)view.to, (int)view.cursor);

	for(int ch=0; ch<8; ch++) {
		Stream stream = streams.get(ch);
		if(!m_channel_map[ch]) continue;

		for(int i=0; i<npoints; i++) {
			int idx_start = ((i+0) * nsamples) / npoints;
			int idx_end   = ((i+1) * nsamples) / npoints;
			float vmin = stream.read(idx_start + view.from);
			float vmax = vmin;
			for(int idx=idx_start; idx<idx_end; idx++) {
				float v = stream.read(idx + view.from);
				vmin = (idx == idx_start || v < vmin) ? v : vmin;
				vmax = (idx == idx_start || v > vmax) ? v : vmax;
			}
			m_waveform.peak = (vmax > m_waveform.peak) ? vmax : m_waveform.peak;
			p_min[i].x = r.x + (npoints - i);
			p_max[i].x = r.x + (npoints - i);
			p_min[i].y = midy - vmin * scale * (r.h / 2);
			p_max[i].y = midy - vmax * scale * (r.h / 2);
			rect[i].x = p_min[i].x;
			rect[i].y = p_max[i].y;
			rect[i].w = 1;
			rect[i].h = p_min[i].y - p_max[i].y;
		}
		ImVec4 col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
		SDL_RenderLines(rend, p_min, npoints);
		SDL_RenderLines(rend, p_max, npoints);
		SDL_SetRenderDrawColor(rend, col.x * 32, col.y * 32, col.z * 32, col.w * 255);
		SDL_RenderFillRects(rend, rect, npoints);
	}

	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = view.idx_to_x(view.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);

	// windwo
	// with the view.count, center at view.cursor

	/*
	if(view.window) {
		const float *wdata = view.window->data();
		SDL_FPoint p[r.w];
		for(size_t x=0; x<(size_t)r.w; x++) {
			size_t idx = view.cursor + view.window->size() - ((x * view.count) / r.w);
			idx = std::clamp(idx, (size_t)0, view.window->size() - 1);
			float v = wdata[idx];
			p[x].x = r.x + r.w - x;
			p[x].y = r.y + r.h - v * r.h;
		}

		SDL_SetRenderDrawColor(rend, 255, 255, 255, 128);
		SDL_RenderLines(rend, p, r.w);
	}
	*/

	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	m_waveform.peak *= 0.9f;
}


void Widget::draw_spectrum(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	bool update = false;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	update |= ImGui::SliderInt("fft size", (int *)&m_spectrum.size, 
				256, 16384, "%d", ImGuiSliderFlags_Logarithmic);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	static const char *k_type_str[] = { "rect", "hanning", "hamming", "blackman", "gauss" };
	update |= ImGui::Combo("window", (int *)&m_spectrum.window_type, 
			                    k_type_str, IM_ARRAYSIZE(k_type_str));

	if(ImGui::IsWindowFocused()) {
		view.window = &m_spectrum.window;
	}
	
	if(update) {
		configure_fft(m_spectrum.size, m_spectrum.window_type);
	}

	if(m_spectrum.window_type == Window::Type::Gauss || 
	   m_spectrum.window_type == Window::Type::Blackman) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		bool changed = ImGui::SliderFloat("sigma", &m_spectrum.window_sigma, 
				0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if(changed) {
			m_spectrum.window.configure(m_spectrum.window_type, 
					                    m_spectrum.size, m_spectrum.window_sigma);
		}
	}

	size_t npoints = m_spectrum.size / 2 + 1;
	SDL_FPoint p[npoints];
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;
		Stream stream = streams.get(ch);

		if(view.cursor < 0) view.cursor = 0;
		if(view.cursor > 1000000) view.cursor = 0;
		stream.read(view.cursor, &m_spectrum.in[0], m_spectrum.size);

		const float *w = m_spectrum.window.data();
		assert(m_spectrum.window.size() == m_spectrum.size);
		for(size_t i=0; i<m_spectrum.size; i++) {
			m_spectrum.in[i] *= w[i];
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
			int h = (int)((v + 100.0f) * (r.h / 150.0f)) - 30;
			p[i].x = r.x + (i * r.w) / npoints;
			p[i].y = r.y + r.h - h;
		}
		ImVec4 col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
		SDL_RenderLines(rend, p, npoints);
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}
