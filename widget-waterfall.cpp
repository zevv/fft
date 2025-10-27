
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget-waterfall.hpp"


WidgetWaterfall::WidgetWaterfall()
	: Widget(Widget::Type::Waterfall)
{
}


WidgetWaterfall::~WidgetWaterfall()
{
}


void WidgetWaterfall::load(ConfigReader::Node *node)
{
	if(!node) return;
	Widget::load(node);
	if(auto *wnode = node->find("waterfall")) {
		if(const char *window_type = wnode->read_str("window_type")) {
			m_view.window_type = Window::str_to_type(window_type);
		}
		wnode->read("window_beta", m_view.window_beta);
		wnode->read("fft_size", m_view.fft_size);
	}
}


void WidgetWaterfall::save(ConfigWriter &cw)
{
	Widget::save(cw);
	cw.push("waterfall");
	cw.write("fft_size", (int)m_view.fft_size);
	cw.write("window_type", Window::type_to_str(m_view.window_type));
	cw.write("window_beta", m_view.window_beta);
	cw.pop();
}


Widget *WidgetWaterfall::do_copy()
{
	WidgetWaterfall *w = new WidgetWaterfall();
	return w;
};


struct Pixel {
	float r, g, b;
};


void WidgetWaterfall::do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	
	size_t stride = 0;
	size_t avail = 0;
	Sample *data = streams.peek(&stride, &avail);
	
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
		ImGui::Text("f=%.6gHz", m_view.freq_cursor * view.srate * 0.5);

		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.freq_cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			} else {
				m_view.freq_cursor = m_view.x_to_freq(pos.x, r);
			}

			auto pos = ImGui::GetIO().MousePos;
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.t_cursor += m_view.dy_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
			} else {
				m_view.t_cursor = m_view.y_to_t(pos.y, r);
			}
		}
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_freq(-ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_freq(ImGui::GetIO().MouseDelta.y);
		}
		ImGuiIO& io = ImGui::GetIO();
		m_view.pan_t(io.MouseWheel * 0.1f);
		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.freq_from = 0.0f;
			m_view.freq_to = 1.0;
		}
	}

	if(m_view.freq_from < 0.0f) m_view.freq_from = 0.0f;
	if(m_view.freq_to > 1.0f) m_view.freq_to = 1.0f;

	m_fft.configure(m_view.fft_size, m_view.window_type, m_view.window_beta);
	m_in.resize(m_view.fft_size);
	
	int fft_w = m_fft.out_size();
	SDL_Texture *tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING, fft_w, r.h);
	uint32_t *pixels;
	int pitch;
	SDL_LockTexture(tex, nullptr, (void **)&pixels, &pitch);
	memset(pixels, 0, pitch * r.h);

	float db_range = -80.0;
	grid_time_v(rend, r, m_view.y_to_t(r.y, r), m_view.y_to_t(r.y + r.h, r));
	
	std::vector<Pixel> row(m_fft.out_size());

	std::vector<Sample> m_in(m_view.fft_size);

	for(int y=0; y<r.h; y++) {
		Time t = m_view.y_to_t(r.y + y, r);
		memset(row.data(), 0, sizeof(Pixel) * row.size());

		for(int ch=0; ch<8; ch++) {
			if(!m_channel_map[ch]) continue;
			SDL_Color col = channel_color(ch);
		
			int idx = (int)(view.srate * t - m_view.fft_size / 2) * stride + ch;
			if(idx < 0) continue;

			for(int i=0; i<m_view.fft_size; i++) {
				m_in[i] = data[idx];
				idx += stride;
			}

			auto m_out_graph = m_fft.run(m_in);

			for(int x=0; x<fft_w; x++) {
				float db = m_out_graph[x];
				float intensity = 0.0;
				if(db > db_range) {
					intensity = (db - db_range) / -db_range;
				}
				SDL_Color col2 = col;
				col2.a = intensity;
				row[x].r += col2.r * intensity;
				row[x].g += col2.g * intensity;
				row[x].b += col2.b * intensity;
			}
		}

		for(int x=0; x<fft_w; x++) {
			if(row[x].r > m_peak) m_peak = row[x].r;
			if(row[x].g > m_peak) m_peak = row[x].g;
			if(row[x].b > m_peak) m_peak = row[x].b;
			uint8_t r = std::min(row[x].r / m_peak * 255, 255.0f);
			uint8_t g = std::min(row[x].g / m_peak * 255, 255.0f);
			uint8_t b = std::min(row[x].b / m_peak * 255, 255.0f);
			pixels[y * (pitch / 4) + x] = (0xff << 24) | (b << 16) | (g << 8) | r;
		}
	}

	m_peak *= 0.95;

	SDL_UnlockTexture(tex);
	SDL_FRect dest;
	SDL_RectToFRect(&r, &dest);

	SDL_FRect src;
	src.x = m_view.freq_from * fft_w;
	src.y = 0;
	src.w = (m_view.freq_to - m_view.freq_from) * fft_w;
	src.h = r.h;

	SDL_RenderTexture(rend, tex, &src, nullptr);
	SDL_DestroyTexture(tex);
	
	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cy = m_view.t_to_y(m_view.t_cursor, r);
	SDL_RenderLine(rend, r.x, cy, r.x + r.w, cy);
	
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.freq_to_x(m_view.freq_cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);


	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


