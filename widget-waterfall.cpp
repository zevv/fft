
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"
#include "widget-waterfall.hpp"


Waterfall::Waterfall()
{
	configure_fft(m_size, m_window_type);
}


Waterfall::~Waterfall()
{
}


void Waterfall::load(ConfigReader::Node *node)
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


void Waterfall::save(ConfigWriter &cw)
{
	cw.push("spectrum");
	cw.write("fft_size", (int)m_size);
	cw.write("window_type", Window::type_to_str(m_window_type));
	cw.write("window_beta", m_window_beta);
	cw.write("freq_from", m_freq_from);
	cw.write("freq_to", m_freq_to);
	cw.pop();
}


void Waterfall::copy_to(Waterfall &w)
{
	w.m_size = m_size;
	w.m_window_type = m_window_type;
	w.m_window_beta = m_window_beta;
	w.configure_fft(w.m_size, w.m_window_type);
};

struct Pixel {
	float r, g, b;
};

void Waterfall::draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
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


			auto pos = ImGui::GetIO().MousePos;
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				view.cursor += dy_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
			} else {
				view.cursor = y_to_t(pos.y, r);
			}
			m_t_cursor = view.cursor;
		}
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			pan(	
					-dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r),
					-dy_to_dt(ImGui::GetIO().MouseDelta.y, r));
			zoom(
					ImGui::GetIO().MouseDelta.x / 100.0f,
					ImGui::GetIO().MouseDelta.y / 100.0f);
		}
	
		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_freq_from = 0.0f;
			m_freq_to = 1.0;

			size_t used;
			size_t stride;
			streams.peek(0, 0, stride, &used);
			m_t_from = 0;
			m_t_to   = used / view.srate;
		}
	}

	if(m_freq_from < 0.0f) m_freq_from = 0.0f;
	if(m_freq_to > 1.0f) m_freq_to = 1.0f;

	
	if(update) {
		configure_fft(m_size, m_window_type);
	}

	auto window = m_window.data();

	int fft_w = window.size() / 2 + 1;
	SDL_Texture *tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING, fft_w, r.h);
	uint32_t *pixels;
	int pitch;
	SDL_LockTexture(tex, nullptr, (void **)&pixels, &pitch);
	memset(pixels, 0, pitch * r.h);

	float db_range = -80.0;
	if(0) {
		widget.grid_time_v(rend, r, y_to_t(r.y, r), y_to_t(r.y + r.h, r));
	}
	
	int ch = 0;
	size_t stride = 0;
	size_t avail = 0;
	Sample *data = streams.peek(ch, 0, stride, &avail);

	std::vector<Pixel> row(m_fft.out_size());

	std::vector<Sample> m_in(m_size);

	for(int y=0; y<r.h; y++) {
		Time t = y_to_t(r.y + y, r);
		memset(row.data(), 0, sizeof(Pixel) * row.size());

		for(int ch=0; ch<8; ch++) {
			if(!widget.channel_enabled(ch)) continue;
			SDL_Color col = widget.channel_color(ch);
		
			int idx = (int)(view.srate * t) * stride + ch;

			for(int i=0; i<m_size; i++) {
				Sample v = 0;
				if(idx >= 0 && idx < (int)(avail * stride)) {
					v = data[idx] * window[i];
				}
				m_in[i] = v;
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
			uint8_t r = std::min(row[x].r, 255.0f);
			uint8_t g = std::min(row[x].g, 255.0f);
			uint8_t b = std::min(row[x].b, 255.0f);
			pixels[y * (pitch / 4) + x] = (0xff << 24) | (b << 16) | (g << 8) | r;
		}
	}

	SDL_UnlockTexture(tex);
	SDL_FRect dest;
	SDL_RectToFRect(&r, &dest);
	SDL_RenderTexture(rend, tex, nullptr, &dest);
	SDL_DestroyTexture(tex);
	
	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cy = t_to_y(view.cursor, r);
	SDL_RenderLine(rend, r.x, cy, r.x + r.w, cy);
	//int cx = freq_to_x(m_freq_cursor, r);
	//SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);


	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


void Waterfall::configure_fft(int size, Window::Type window_type)
{
	m_fft.set_window(m_window_type, m_window_beta);
	m_fft.set_size(size);

	m_size = size;
	m_in.resize(size);
	m_out_graph.resize(size);

	m_window.configure(window_type, size);
}

