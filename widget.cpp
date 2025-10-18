
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"
		


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


Widget::~Widget()
{
	if(m_spectrum.plan) {
		fftwf_destroy_plan(m_spectrum.plan);
		m_spectrum.plan = nullptr;
	}
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


Widget *Widget::copy(void)
{
	Widget *w = new Widget(m_type);
	for(int i=0; i<8; i++) {
		w->m_channel_map[i] = m_channel_map[i];
	}
	w->m_waveform.agc = m_waveform.agc;
	w->m_waveform.peak = m_waveform.peak;
	w->m_spectrum.size = m_spectrum.size;
	w->m_spectrum.window_type = m_spectrum.window_type;
	w->m_spectrum.window_beta = m_spectrum.window_beta;
	w->configure_fft(w->m_spectrum.size, w->m_spectrum.window_type);
	return w;
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

	m_has_focus = ImGui::IsWindowFocused();

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


float Widget::draw_graph(View &view, SDL_Renderer *rend, SDL_Rect &r, 
					 ImVec4 &col, float *data, size_t stride,
					 int idx_from, int idx_to,
					 float y_min, float y_max)
{
	int nsamples = view.wave_to - view.wave_from;

	SDL_FPoint p_max[r.w + 2];
	SDL_FPoint p_min[r.w + 2];
	SDL_FRect rects[r.w + 2];

	float y_off = r.y + r.h / 2.0f;
	float y_scale = (r.h - 2) / (y_max - y_min);
	float v_peak = 0.0;

	int npoints = 0;
	int nrects = 0;

	for(int x=0; x<r.w; x+=2) {

		int idx_start = idx_from + ((x + 0) * (idx_to - idx_from)) / r.w;
		int idx_end   = idx_from + ((x + 2) * (idx_to - idx_from)) / r.w;

		float vmin = data[stride * idx_start];
		float vmax = data[stride * idx_start];

		for(int idx=idx_start; idx<idx_end; idx++) {
			float v = data[stride * idx];
			vmin = (idx == idx_start || v < vmin) ? v : vmin;
			vmax = (idx == idx_start || v > vmax) ? v : vmax;
		}

		p_min[npoints].x = r.x + x;
		p_min[npoints].y = vmin * y_scale + y_off;
		p_max[npoints].x = r.x + x;
		p_max[npoints].y = vmax * y_scale + y_off;
		npoints++;

		if(vmax > vmin) {
			rects[nrects].x = r.x + x;
			rects[nrects].y = p_max[npoints - 1].y;
			rects[nrects].w = 2;
			rects[nrects].h = p_min[npoints - 1].y - p_max[npoints - 1].y;
			nrects++;
		}

		if(x == 0 || vmax >  v_peak) v_peak =  vmax;
		if(x == 0 || vmin < -v_peak) v_peak = -vmin;
	}

	SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
	SDL_RenderLines(rend, p_max, npoints);
	SDL_RenderLines(rend, p_min, npoints);
	SDL_SetRenderDrawColor(rend, col.x * 48, col.y * 48, col.z * 48, col.w * 255);
	SDL_RenderFillRects(rend, rects, nrects);

	return v_peak;
}


ImVec4 Widget::channel_color(int channel)
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


const char *k_type_str[] = {
	"none", "wave", "spectrum", "waterfall"
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

