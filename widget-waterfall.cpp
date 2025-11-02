
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "widgetinfo.hpp"
#include "widget-waterfall.hpp"
#include "histogram.hpp"


WidgetWaterfall::WidgetWaterfall(WidgetInfo &info)
	: Widget(info)
	, m_fft(true)
{
}


WidgetWaterfall::~WidgetWaterfall()
{
}


struct Pixel {
	float r, g, b;
};


void WidgetWaterfall::do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	
	size_t stride = 0;
	size_t frames_avail = 0;
	Sample *data = streams.peek(&stride, &frames_avail);
	
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
	
	ImGui::SameLine();
	ImGui::ToggleButton("A##pproximate FFT", &m_fft_approximate);
	m_fft.set_approximate(m_fft_approximate);

	ImGui::SameLine();
	ImGui::ToggleButton("AGC", &m_agc);

	float db_min = -100.0;
	float db_max = 0.0;

	if(!m_agc) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db center", &m_view.aperture.center, 0.0f, -100.0f, "%.1f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db range", &m_view.aperture.range, 100.0f, 0.0f, "%.1f");
		db_min = m_view.aperture.center - m_view.aperture.range;
		db_max = m_view.aperture.center + m_view.aperture.range;
	} else {
		db_min = m_db_min;
		db_max = m_db_max;
	}

	if(has_focus()) {

		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("f=%.6gHz", m_view.freq.cursor * m_view.srate * 0.5);

		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.freq.cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			} else {
				m_view.freq.cursor = m_view.x_to_freq(pos.x, r);
			}
		
			if(ImGui::GetIO().MouseDelta.y != 0) {
				auto pos = ImGui::GetIO().MousePos;
				if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
					m_view.time.cursor += m_view.dy_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
				} else {
					m_view.time.cursor = m_view.y_to_t(pos.y, r);
				}
			}
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

	int fft_w = m_fft.out_size();

	Histogram hist(64, -120.0, 0.0);

	m_db_min = 0.0;
	m_db_max = -200.0;

	SDL_FRect src;
	src.x = m_view.freq.from * fft_w;
	src.y = 0;
	src.w = (m_view.freq.to - m_view.freq.from) * fft_w;
	src.h = 1;

	SDL_FRect dst;
	dst.x = r.x;
	dst.y = 0;
	dst.w = r.w;
	dst.h = 1.0f;

	SDL_Texture *tex = SDL_CreateTexture(
			rend, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, fft_w, 1);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
	SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);

	for(int y=0; y<r.h; y++) {
		Time t = m_view.y_to_t(r.y + y, r);

		for(int ch : m_channel_map.enabled_channels()) {

			int idx = (int)(m_view.srate * t - m_view.fft.size / 2) * stride + ch;
			if(idx < 0) continue;
			if(idx >= (int)(frames_avail * stride)) break;

			auto fft_out = m_fft.run(&data[idx], stride);
			SDL_Color col = m_channel_map.ch_color(ch);
			uint32_t v = *(uint32_t *)&col & 0x00FFFFFF;

			uint32_t *pixels;
			int pitch;
			SDL_LockTexture(tex, nullptr, (void **)&pixels, &pitch);
			memset(pixels, 0, pitch);

			for(int x=0; x<fft_w; x++) {
				float db = fft_out[x];
				hist.add(db);
				m_db_min = std::min(m_db_min, db);
				m_db_max = std::max(m_db_max, db);
				float intensity = 0.0;
				if(db > db_min && db_max > db_min) {
					intensity = std::clamp((db - db_min) / (db_max - db_min), 0.0f, 1.0f);
				}
				pixels[x] = v | (uint32_t)(intensity * 255) << 24;
			}

			SDL_UnlockTexture(tex);
			dst.y = r.y + y;
			SDL_RenderTexture(rend, tex, &src, &dst);
		}

	}

	SDL_DestroyTexture(tex);

	if(m_agc) {
		m_db_min = hist.get_percentile(0.05);
		m_db_max = hist.get_percentile(1.00);
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	// grid
	//grid_time_v(rend, r, m_view.time.from, m_view.time.to);

	// time cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cy = m_view.t_to_y(m_view.time.cursor, r);
	SDL_RenderLine(rend, r.x, cy, r.x + r.w, cy);
	
	// play position
	SDL_SetRenderDrawColor(rend, 0, 128, 128, 255);
	int py = m_view.t_to_y(m_view.time.playpos, r);
	SDL_RenderLine(rend, r.x, py, r.x + r.w, py);

	// freq cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.freq_to_x(m_view.freq.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetWaterfall,
	.name = "waterfall",
	.description = "FFT spectrum waterfall",
	.hotkey = ImGuiKey_F3,
);

