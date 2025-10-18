
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
		.window_type = Window::Type::Hanning,
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


void Widget::load(ConfigReader::Node *n)
{
	if(const char *type = n->read_str("type")) {

		int channel_map = 0;
		if(n->read("channel_map", channel_map)) {
			for(int i=0; i<8; i++) {
				m_channel_map[i] = (channel_map & (1 << i)) ? true : false;
			}
		}

		if(strcmp(type, "none") == 0) {
			m_type = Type::None;
		}

		if(strcmp(type, "wave") == 0) {
			m_type = Type::Waveform;
			if(auto *nc = n->find("waveform")) {
				nc->read("agc", m_waveform.agc, true);
			}
		}

		if(strcmp(type, "spectrum") == 0) {
			if(auto nc = n->find("spectrum")) {
				m_type = Type::Spectrum;
				if(const char *window_type = nc->read_str("window_type")) {
					m_spectrum.window_type = Window::str_to_type(window_type);
				}
				nc->read("window_beta", m_spectrum.window_beta);
				nc->read("fft_size", m_spectrum.size);
				configure_fft(m_spectrum.size, m_spectrum.window_type);
			}
		}
	}
}


void Widget::save(ConfigWriter &cw)
{
	cw.push("widget");

	int channel_map = 0;
	for(int i=0; i<8; i++) {
		if(m_channel_map[i]) channel_map |= (1 << i);
	}

	cw.write("type", Widget::type_to_string(m_type));
	cw.write("channel_map", channel_map);

	if(m_type == Type::Waveform) {
		cw.push("waveform");
		cw.write("agc", m_waveform.agc);
		cw.pop();
	}

	if(m_type == Type::Spectrum) {
		cw.push("spectrum");
		cw.write("fft_size", (int)m_spectrum.size);
		cw.write("window_type", Window::type_to_str(m_spectrum.window_type));
		cw.write("window_beta", m_spectrum.window_beta);
		cw.pop();
	}

	cw.pop();

}


void Widget::configure_fft(int size, Window::Type window_type)
{
	if(m_spectrum.plan) {
		fftwf_destroy_plan(m_spectrum.plan);
		m_spectrum.plan = nullptr;
	}
	m_spectrum.size = size;
	m_spectrum.in.resize(size);
	m_spectrum.out.resize(size);

	int sizes[] = {size};
	fftwf_r2r_kind kinds[] = { FFTW_R2HC };
	m_spectrum.plan = fftwf_plan_many_r2r(
			1, sizes, 1, 
			m_spectrum.in.data(), NULL, 1, 0, 
			m_spectrum.out.data(), NULL, 1, 0, 
			kinds, FFTW_ESTIMATE);

	m_spectrum.window.configure(window_type, size);
}


void Widget::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
{
	ImGui::SetNextItemWidth(100);
	ImGui::Combo("type", (int *)&m_type, 
			Widget::type_names(), Widget::type_count());

	// Channel enable buttons
	for(size_t i=0; i<8; i++) {
		ImGui::SameLine();
		ImVec4 col = m_channel_map[i] ? channel_color(i) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		char label[2] = { (char)('1' + i), 0 };
		ImGuiKey key = (ImGuiKey)(ImGuiKey_1 + i);
		if(ImGui::SmallButton(label) || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(key))) {
			// with shift: solo, otherwise toggle
			if(ImGui::GetIO().KeyShift) for(int j=0; j<8; j++) m_channel_map[j] = 0;
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
	int nsamples = view.wave_to - view.wave_from;
	SDL_FPoint p_min[npoints];
	SDL_FPoint p_max[npoints];
	SDL_FRect rect[npoints];

	ImGui::SameLine();
	ImGui::Text("| peak: %.2f", m_waveform.peak);

	for(int ch=0; ch<8; ch++) {
		Stream stream = streams.get(ch);
		if(!m_channel_map[ch]) continue;

		int nrects = 0;
		for(int i=0; i<npoints; i++) {
			int idx_start = ((i+0) * nsamples) / npoints;
			int idx_end   = ((i+1) * nsamples) / npoints;
			float vmin = stream.read(idx_start + view.wave_from);
			float vmax = vmin;
			for(int idx=idx_start; idx<idx_end; idx++) {
				float v = stream.read(idx + view.wave_from);
				vmin = (idx == idx_start || v < vmin) ? v : vmin;
				vmax = (idx == idx_start || v > vmax) ? v : vmax;
			}
			m_waveform.peak = (vmax > m_waveform.peak) ? vmax : m_waveform.peak;
			p_min[i].x = r.x + (npoints - i);
			p_max[i].x = r.x + (npoints - i);
			p_min[i].y = midy - vmin * scale * (r.h / 2);
			p_max[i].y = midy - vmax * scale * (r.h / 2);
			
			int rh = p_min[i].y - p_max[i].y;
			if(rh > 0) {
				rect[nrects].x = p_min[i].x;
				rect[nrects].y = p_max[i].y;
				rect[nrects].w = 1;
				rect[nrects].h = p_min[i].y - p_max[i].y;
				nrects++;
			}
		}
		ImVec4 col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
		SDL_RenderLines(rend, p_min, npoints);
		SDL_RenderLines(rend, p_max, npoints);
		SDL_SetRenderDrawColor(rend, col.x * 32, col.y * 32, col.z * 32, col.w * 255);
		SDL_RenderFillRects(rend, rect, nrects);
	}

	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = view.idx_to_x(view.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);

	// window
	if(view.window) {
		size_t wsize = view.window->size();
		const float *wdata = view.window->data();
		SDL_FPoint p[66];
		p[0].x = view.idx_to_x(view.cursor, r);
		p[0].y = r.y + r.h - 1;
		p[65].x = view.idx_to_x(view.cursor + wsize, r);
		p[65].y = r.y + r.h - 1;
		for(int i=0; i<64; i++) {
			int n = wsize * i / 64;
			p[i+1].x = view.idx_to_x(view.cursor + wsize * i / 63.0, r);
			p[i+1].y = r.y + (r.h-1) * (1.0f - wdata[n]);
		}

		SDL_SetRenderDrawColor(rend, 255, 255, 255, 128);
		SDL_RenderLines(rend, p, 66);
	}

	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	m_waveform.peak *= 0.9f;
}


void Widget::draw_spectrum(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	bool update = false;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	update |= ImGui::SliderInt("fft size", (int *)&m_spectrum.size, 
				16, 16384, "%d", ImGuiSliderFlags_Logarithmic);

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

	// 0 .. -100 dB
	for(float dB=-100.0f; dB<=0.0f; dB+=10.0f) {
		int y = view.db_to_y(dB, r);
		SDL_RenderLine(rend, r.x, y, r.x + r.w, y);
		char buf[32];
		snprintf(buf, sizeof(buf), "%+.0f", dB);
		dl->AddText(ImVec2((float)r.x + 4, y), ImColor(64, 64, 64, 255), buf);
	}
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	// spectorams

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;
		Stream stream = streams.get(ch);

		memset(&m_spectrum.in[0], 0, sizeof(float) * m_spectrum.size);
		stream.read(view.cursor, &m_spectrum.in[0], m_spectrum.size);

		const float *w = m_spectrum.window.data();
		for(int i=0; i<m_spectrum.size; i++) {
			m_spectrum.in[i] *= w[i];
		}

		fftwf_execute(m_spectrum.plan);

		float scale = 2.0f / (float)m_spectrum.size * m_spectrum.window.gain();

		for(int i=0; i<m_spectrum.size; i++) {
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
			m_spectrum.out[i] = v * scale;
		}

		for(size_t i=0; i<npoints; i++) {
			float v = m_spectrum.out[i];
			float dB = (v > 1e-6f) ? 20.0f * log10f(v) : -100.0f;
			p[i].x = r.x + (i * r.w) / (npoints - 1);
			p[i].y = view.db_to_y(dB, r);
		}
		ImVec4 col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
		SDL_RenderLines(rend, p, npoints);

		if(ch == 2) {
			FILE *f = fopen("/tmp/spectrum", "w");
			for(size_t i=0; i<npoints; i++) {
				fprintf(f, "%f\n", 20.0f * log10f(m_spectrum.out[i] + 1e-6f));
			}
			fclose(f);
		}
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


const char *k_type_str[] = {
	"none", "spectrum", "waterfall", "wave"
};

const char **Widget::type_names()
{
	return k_type_str;
}

size_t Widget::type_count()
{
	return IM_ARRAYSIZE(k_type_str);
}

const char *Widget::type_to_string(Type type)
{
	size_t count = type_count();
	const char **names = type_names();
	for(size_t i=0; i<count; i++) {
		if((Type)i == type) return names[i];
	}
	return "none";
}


Widget::Type Widget::string_to_type(const char *str)
{
	size_t count = type_count();
	const char **names = type_names();
	for(size_t i=0; i<count; i++) {
		if(strcmp(names[i], str) == 0) return (Type)i;
	}
	return Type::None;
}

