
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "widgetregistry.hpp"
#include "widget-waterfall.hpp"
#include "histogram.hpp"


WidgetWaterfall::WidgetWaterfall(Widget::Info &info)
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


void WidgetWaterfall::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	
	size_t stride = 0;
	size_t frames_avail = 0;
	Sample *data = stream.peek(&stride, &frames_avail);
	
	ImGui::SameLine();
	ImGui::ToggleButton("A##pproximate FFT", &m_fft_approximate);
	m_fft.set_approximate(m_fft_approximate);

	ImGui::SameLine();
	ImGui::ToggleButton("AGC", &m_agc);

	int8_t db_min = -100.0;
	int8_t db_max = 0.0;

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

	if(ImGui::IsWindowFocused()) {

		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		float f = m_view.freq.cursor * stream.sample_rate() * 0.5f;
		char note[32];
		freq_to_note(f, note, sizeof(note));
		ImGui::Text("f=%.6gHz %s", f, note);

		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.freq.cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			} else {
				m_view.freq.cursor = m_view.x_to_freq(pos.x, r);
			}
		
		}
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.pan_freq(-ImGui::GetIO().MouseDelta.x / r.w);
				m_view.zoom_freq(ImGui::GetIO().MouseDelta.y);
			} else {
				m_view.pan_t(ImGui::GetIO().MouseDelta.y / r.w);
				m_view.zoom_t(ImGui::GetIO().MouseDelta.x);
			}
		}


		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.freq.from = 0.0f;
			m_view.freq.to = 1.0;
			m_view.time.from = 0;
			m_view.time.to   = frames_avail / stream.sample_rate();
		}

		if(ImGui::IsMouseInRect(r)) {
		   if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			   stream.player.seek(m_view.y_to_t(pos.y, r));
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
	}

	m_fft.configure(m_view.window.size, m_view.window.window_type, m_view.window.window_beta);

	int fft_w = m_fft.out_size();

	Histogram hist(64, -120.0, 0.0);

	m_db_min =   -0;
	m_db_max = -120;

	SDL_FRect src;
	src.x = m_view.freq.from * fft_w;
	src.y = 0;
	src.w = (m_view.freq.to - m_view.freq.from) * fft_w;
	src.h = r.h;

	SDL_FRect dst;
	dst.x = r.x;
	dst.y = r.y;
	dst.w = r.w;
	dst.h = r.h;

	SDL_Texture *tex = SDL_CreateTexture(
			rend, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, fft_w, r.h);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
	SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
		
	for(int ch : m_channel_map.enabled_channels()) {
			
		SDL_Color col = m_channel_map.ch_color(ch);

		uint32_t *pixels;
		int pitch;
		SDL_LockTexture(tex, nullptr, (void **)&pixels, &pitch);
		memset(pixels, 0, pitch * r.h);

		for(int y=0; y<r.h; y++) {
			Time t = m_view.y_to_t(r.y + y, r);
			int idx = (int)(stream.sample_rate() * t - m_view.window.size / 2) * stride + ch;
			if(idx < 0) continue;
			if(idx >= (int)(frames_avail * stride)) break;

			auto fft_out = m_fft.run(&data[idx], stride);
			uint32_t v = *(uint32_t *)&col & 0x00FFFFFF;
			uint32_t *p = pixels + y * (pitch / 4);

			for(int x=0; x<fft_w; x++) {
				int8_t db = fft_out[x];
				hist.add(db);
				m_db_min = std::min(m_db_min, db);
				m_db_max = std::max(m_db_max, db);
				int8_t intensity = 0.0;
				if(db > db_min && db_max > db_min) {
					intensity = std::clamp(255 * (db - db_min) / (db_max - db_min), 0, 255);
				}
				*p++ = v | (uint32_t)(intensity << 24);
			}

		}
		SDL_UnlockTexture(tex);
		SDL_RenderTexture(rend, tex, &src, &dst);
	}

	SDL_DestroyTexture(tex);

	if(m_agc) {
		m_db_min = hist.get_percentile(0.05);
		m_db_max = hist.get_percentile(1.00);
	}


	// darken filtered out area
	float f_lp, f_hp;
	stream.player.filter_get(f_lp, f_hp);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_FRect filt_rect;
	filt_rect.y = r.y;
	filt_rect.h = r.h;
	int x_hp = m_view.freq_to_x(f_hp, r);
	filt_rect.x = r.x;
	filt_rect.w = x_hp - r.x;
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 128);
	SDL_RenderFillRect(rend, &filt_rect);
	int x_lp = m_view.freq_to_x(f_lp, r);
	filt_rect.x = x_lp;
	filt_rect.w = r.x + r.w - x_lp;
	SDL_RenderFillRect(rend, &filt_rect);

	// selection
	if(m_view.time.sel_from != m_view.time.sel_to) {
		float sx_from = m_view.t_to_y(m_view.time.sel_from, r);
		float sx_to   = m_view.t_to_y(m_view.time.sel_to,   r);
		SDL_SetRenderDrawColor(rend, 128, 128, 255, 64);
		SDL_FRect sr = { (float)r.x, sx_from, (float)r.w, sx_to - sx_from };
		SDL_RenderFillRect(rend, &sr);
	}
	

	// grid
	//grid_time_v(rend, r, m_view.time.from, m_view.time.to);

	// cursors
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	int cx = m_view.freq_to_x(m_view.freq.cursor, r);
	int cy = m_view.t_to_y(m_view.time.cursor, r);
	vcursor(rend, r, cy, false);
	hcursor(rend, r, cx, false);
	vcursor(rend, r, m_view.t_to_y(m_view.time.playpos, r), true);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetWaterfall,
	.name = "waterfall",
	.description = "FFT spectrum waterfall",
	.hotkey = ImGuiKey_F3,
	.flags = Widget::Info::Flags::ShowChannelMap | 
	         Widget::Info::Flags::ShowLock |
			 Widget::Info::Flags::ShowWindowSize |
			 Widget::Info::Flags::ShowWindowType,
);

