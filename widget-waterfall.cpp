
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
			m_view.fft.window_type = Window::str_to_type(window_type);
		}
		wnode->read("window_beta", m_view.fft.window_beta);
		wnode->read("fft_size", m_view.fft.size);
	}
}


void WidgetWaterfall::save(ConfigWriter &cw)
{
	Widget::save(cw);
	cw.push("waterfall");
	cw.write("fft_size", (int)m_view.fft.size);
	cw.write("window_type", Window::type_to_str(m_view.fft.window_type));
	cw.write("window_beta", m_view.fft.window_beta);
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


static void find_aperture(std::array<size_t, 256> hist, float &db_min, float &db_max)
{
	size_t total = 0;
	for(auto &v : hist) total += v;
	size_t count_05 = total * 0.05;
	size_t count_99 = total * 0.99;
	size_t cum = 0;
	db_min = db_max = 1.0;
	for(size_t i=0; i<hist.size(); i++) {
		cum += hist[i];
		if(db_max == 1.0 && cum >= count_05) db_max = -(float)i;
		if(db_min == 1.0 && cum >= count_99) db_min = -(float)i;
	}
	db_max = std::min(db_max, -1.0f);
	db_min = std::max(db_min, db_max - 80.0f);
}


void WidgetWaterfall::do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
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
		ImGui::Text("f=%.6gHz", m_view.freq.cursor * view.srate * 0.5);

		auto pos = ImGui::GetIO().MousePos;
		if(pos.x >= 0) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.freq.cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			} else {
				m_view.freq.cursor = m_view.x_to_freq(pos.x, r);
			}

			auto pos = ImGui::GetIO().MousePos;
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.time.cursor += m_view.dy_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
			} else {
				m_view.time.cursor = m_view.y_to_t(pos.y, r);
			}
		}
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_freq(-ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_freq(ImGui::GetIO().MouseDelta.y);
		}
		ImGuiIO& io = ImGui::GetIO();
		m_view.pan_t(io.MouseWheel * 0.1f);
		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.freq.from = 0.0f;
			m_view.freq.to = 1.0;
		}
	}

	m_fft.configure(m_view.fft.size, m_view.fft.window_type, m_view.fft.window_beta);
	
	int fft_w = m_fft.out_size();
	SDL_Texture *tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING, fft_w, r.h);
	uint32_t *pixels;
	int pitch;
	SDL_LockTexture(tex, nullptr, (void **)&pixels, &pitch);
	memset(pixels, 0, pitch * r.h);

	grid_time_v(rend, r, m_view.time.from, m_view.time.to);
	
	std::vector<Pixel> row(m_fft.out_size());

	std::array<size_t, 256> hist{};

	m_db_min = 0.0;
	m_db_max = -200.0;

	for(int y=0; y<r.h; y++) {
		Time t = m_view.y_to_t(r.y + y, r);
		memset(row.data(), 0, sizeof(Pixel) * row.size());

		for(int ch=0; ch<8; ch++) {
			if(!m_channel_map[ch]) continue;
			SDL_Color col = channel_color(ch);
	
			int idx = (int)(view.srate * t - m_view.fft.size / 2) * stride + ch;
			if(idx < 0) continue;
			if(idx >= (int)(frames_avail * stride)) break;

			auto out_graph = m_fft.run(&data[idx], stride);

			for(int x=0; x<fft_w; x++) {
				float db = out_graph[x];
				int bin = -db;
				if(bin > 0 && bin < (int)hist.size()) hist[bin]++;
				m_db_min = std::min(m_db_min, db);
				m_db_max = std::max(m_db_max, db);
				float intensity = 0.0;
				if(db > db_min && db_max > db_min) {
					intensity = (db - db_min) / (db_max - db_min);
				}
				SDL_Color col2 = col;
				col2.a = intensity;
				row[x].r += col2.r * intensity;
				row[x].g += col2.g * intensity;
				row[x].b += col2.b * intensity;
			}
		}

		for(int x=0; x<fft_w; x++) {
			uint8_t r = std::min(row[x].r, 255.0f);
			uint8_t g = std::min(row[x].g, 255.0f);
			uint8_t b = std::min(row[x].b, 255.0f);
			pixels[y * (pitch / 4) + x] = (0xff << 24) | (b << 16) | (g << 8) | r;
		}
	}

	if(m_agc) {
		find_aperture(hist, m_db_min, m_db_max);
	}

	SDL_UnlockTexture(tex);
	SDL_FRect dest;
	SDL_RectToFRect(&r, &dest);

	SDL_FRect src;
	src.x = m_view.freq.from * fft_w;
	src.y = 0;
	src.w = (m_view.freq.to - m_view.freq.from) * fft_w;
	src.h = r.h;

	SDL_FRect dst;
	SDL_RectToFRect(&r, &dst);
	SDL_RenderTexture(rend, tex, &src, &dst);
	SDL_DestroyTexture(tex);
	
	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cy = m_view.t_to_y(m_view.time.cursor, r);
	SDL_RenderLine(rend, r.x, cy, r.x + r.w, cy);
	
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.freq_to_x(m_view.freq.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);


	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}
