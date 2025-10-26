
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget-wave.hpp"
#include "widget-spectrum.hpp"
#include "widget-waterfall.hpp"
#include "widget-histogram.hpp"
#include "widget.hpp"


Widget::Widget(Type type)
	: m_type(type)
{
}


Widget::~Widget()
{
}


Widget *Widget::create(Widget::Type type)
{
	if(type == Widget::Type::None) return new WidgetNone();
	if(type == Widget::Type::Waveform) return new WidgetWaveform();
	if(type == Widget::Type::Spectrum) return new WidgetSpectrum();
	if(type == Widget::Type::Waterfall) return new WidgetWaterfall();
	if(type == Widget::Type::Histogram) return new WidgetHistogram();
	if(type == Widget::Type::StyleEditor) return new WidgetStyleEditor();
	assert(false && "unknown widget type");
	return nullptr;
}


Widget *Widget::create(const char *type_str)
{
	Widget::Type type = Widget::string_to_type(type_str);
	return Widget::create(type);
}


void Widget::load(ConfigReader::Node *node)
{
	if(!node) return;
	int channel_map = 0;
	if(node->read("channel_map", channel_map)) {
		for(int i=0; i<8; i++) {
			m_channel_map[i] = (channel_map & (1 << i)) ? true : false;
		}
	}
	m_view.load(node);
}


void Widget::save(ConfigWriter &cw)
{
	int channel_map = 0;
	for(int i=0; i<8; i++) {
		if(m_channel_map[i]) channel_map |= (1 << i);
	}

	cw.write("widget", Widget::type_to_string(m_type));
	cw.write("channel_map", channel_map);
	cw.write("lock_view", m_lock_view);
	m_view.save(cw);
}


Widget *Widget::copy()
{
	Widget *w = do_copy();
	copy_to(w);
	return w;
}


void Widget::copy_to(Widget *w)
{
	w->m_lock_view = m_lock_view;
	w->m_view = m_view;
	for(int i=0; i<8; i++) {
		w->m_channel_map[i] = m_channel_map[i];
	}
}


void Widget::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	m_has_focus = ImGui::IsWindowFocused();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
	for(size_t i=0; i<8; i++) {
		if(i > 0) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		}

		ImGui::SameLine();
		SDL_Color c = Widget::channel_color(i);
		ImVec4 col = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		if(m_channel_map[i]) col = ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		char label[2] = { (char)('1' + i), 0 };
		ImGuiKey key = (ImGuiKey)(ImGuiKey_1 + i);
		if(ImGui::Button(label) || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(key))) {
			if(ImGui::GetIO().KeyShift) for(int j=0; j<8; j++) m_channel_map[j] = 0;
			m_channel_map[i] = !m_channel_map[i];
		}
		ImGui::PopStyleColor(3);
		if(i > 0) ImGui::PopStyleVar(1);
	}
	ImGui::PopStyleVar(1);

	ImGui::SameLine();
	ImGui::ToggleButton("Lock", &m_lock_view);
	
	if(m_has_focus) {
		if(ImGui::IsKeyPressed(ImGuiKey_0)) {
			for(int i=0; i<8; i++) m_channel_map[i] ^= 1;
		}
		if(ImGui::IsKeyPressed(ImGuiKey_L)) m_lock_view = !m_lock_view;
	}

	if(m_lock_view) m_view = view;
	float t1 = SDL_GetPerformanceCounter();
	do_draw(view, streams, rend, r);
	float t2 = SDL_GetPerformanceCounter();
	if(m_lock_view) view = m_view;

	// bottom right
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 60, ImGui::GetWindowHeight() - 25));
	ImGui::Text("%.2f ms", (t2 - t1) * 1000.0f / SDL_GetPerformanceFrequency());
}


#ifndef SAMPLE_FLOAT
template float  Widget::graph<float >(SDL_Renderer*, SDL_Rect&, float [],           size_t, size_t, float , float , float , float );
template float  Widget::graph<float >(SDL_Renderer*, SDL_Rect&, float [], float [], size_t, size_t, float , float , float , float );
#endif

template Sample Widget::graph<Sample>(SDL_Renderer*, SDL_Rect&, Sample[],           size_t, size_t, float, float, Sample, Sample);
template Sample Widget::graph<Sample>(SDL_Renderer*, SDL_Rect&, Sample[], Sample[], size_t, size_t, float, float, Sample, Sample);

template int    Widget::graph<int   >(SDL_Renderer*, SDL_Rect&, int   [],           size_t, size_t, float, float, int   , int   );
template int    Widget::graph<int   >(SDL_Renderer*, SDL_Rect&, int   [], int   [], size_t, size_t, float, float, int   , int   );

template<typename T>
T Widget::graph(SDL_Renderer *rend, SDL_Rect &r,
					 T data[], size_t data_count, size_t stride,
					 float idx_from, float idx_to,
					 T y_min, T y_max)
{
	return graph(rend, r, data, data, data_count, stride,
		  idx_from, idx_to,
		  y_min, y_max);
}

template<typename T>
T Widget::graph(SDL_Renderer *rend, SDL_Rect &r,
					 T data_min[], T data_max[], size_t data_count, size_t stride,
					 float idx_from, float idx_to,
					 T y_min, T y_max)
{
	float y_scale = (r.h - 2) / ((float)y_min - (float)y_max);
	float y_off = r.y - (float)y_max * (float)y_scale;
	T v_peak = 0;

	int npoints = 0;
	int nrects = 0;
	int step = 2;
	float idx_scale = (idx_to - idx_from) / r.w;
	int max_points = r.w / step + 1;

	int idx_min = 0;
	int idx_max = data_count;

	std::vector<SDL_FPoint> p_max(max_points);
	std::vector<SDL_FPoint> p_min(max_points);
	std::vector<SDL_FRect> rects(max_points);

	for(int x=0; x<r.w; x+=step) {

		int idx_start = idx_from + (x + 0   ) * idx_scale;
		int idx_end   = idx_from + (x + step) * idx_scale;

		if(idx_end   >= idx_max) break;
		if(idx_start <  idx_min) continue;

		T vmin = data_min[stride * idx_start];
		T vmax = data_max[stride * idx_start];

		for(int idx=idx_start; idx<idx_end; idx++) {
			vmin = std::min(vmin, data_min[stride * idx]);
			vmax = std::max(vmax, data_max[stride * idx]);
		}

		float px = r.x + x;
		float py_min = vmin  * y_scale + y_off;
		float py_max = vmax  * y_scale + y_off;

		p_min[npoints].x = px;
		p_min[npoints].y = py_min;
		p_max[npoints].x = px;
		p_max[npoints].y = py_max;
		npoints++;

		if(py_max < py_min - 4) {
			rects[nrects].x = px;
			rects[nrects].y = py_max + 1;
			rects[nrects].w = step;
			rects[nrects].h = py_min - py_max - 1;
			nrects++;
		}

		if(x == 0 || vmax >  v_peak) v_peak =  vmax;
		if(x == 0 || vmin < -v_peak) v_peak = -vmin;
	}

	SDL_RenderLines(rend, p_max.data(), npoints);
	SDL_RenderLines(rend, p_min.data(), npoints);
	uint8_t cr, cg, cb, ca;
	SDL_GetRenderDrawColor(rend, &cr, &cg, &cb, &ca);
	SDL_SetRenderDrawColor(rend, cr/5, cg/5, cb/5, ca);
	SDL_RenderFillRects(rend, rects.data(), nrects);

	return v_peak;
}


struct Unit {
	double base;
	const char *unit;
	double scale;
	const char *fmt;
} units[] = {
	{ 1e5,   "k", 1e-3, "%.0fk" },
	{ 1e4,   "k", 1e-3, "%.0fk" },
	{ 1e3,   "k", 1e-3, "%.0fk" },
	{ 1e2,   "",  1e0,  "%.0f"  },
	{ 1e1,   "",  1e0,  "%.0f"  },
	{ 1e0,   "",  1e0,  "%.0f"  },
	{ 1e-1,  "",  1e0,  "%.1f" },
	{ 1e-2,  "",  1e3,  "%.0fm" },
	{ 1e-3,  "",  1e3,  "%.0fm" },
	{ 1e-4,  "",  1e3,  "%.1fm" },
};

void Widget::grid_time(SDL_Renderer *rend, SDL_Rect &r, Time t_min, Time t_max)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	for(auto &u : units) {
		double dx = r.w * u.base / (t_max - t_min);
		if(dx < 10) break;
		int col = std::clamp((int)(dx-10) * 4, 0, 64);
		SDL_SetRenderDrawColor(rend, col, col, col, 255);
		Time t_start = ceilf(t_min / u.base) * u.base;
		Time t_end   = floorf(t_max / u.base) * u.base;
		Time t = t_start;
		while(t <= t_end) {
			int x = r.x + (int)((t - t_min) / (t_max - t_min) * r.w);
			SDL_RenderLine(rend, x, r.y, x, r.y + r.h);
			if(dx > 50) {
				char buf[32];
				snprintf(buf, sizeof(buf), u.fmt, fabs(t * u.scale));
				dl->AddText(ImVec2(x + 2, r.y - 3), 0xFF808080, buf);
			}
			t += u.base;
		}
	}
}

void Widget::grid_time_v(SDL_Renderer *rend, SDL_Rect &r, Time t_min, Time t_max)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	for(auto &u : units) {
		double dy = r.h * u.base / (t_max - t_min);
		if(dy < 10) break;
		int col = std::clamp((int)(dy-10) * 4, 0, 64);
		SDL_SetRenderDrawColor(rend, col, col, col, 255);
		Time t_start = ceilf(t_min / u.base) * u.base;
		Time t_end   = floorf(t_max / u.base) * u.base;
		Time t = t_start;
		while(t <= t_end) {
			int y = r.y + (int)((t - t_min) / (t_max - t_min) * r.h);
			SDL_RenderLine(rend, r.x, y, r.x + r.w, y);
			if(dy > 20) {
				char buf[32];
				snprintf(buf, sizeof(buf), u.fmt, fabs(t * u.scale));
				dl->AddText(ImVec2(r.x + 2, y - 3), 0xFF808080, buf);
			}
			t += u.base;
		}
	}
}


void Widget::grid_vertical(SDL_Renderer *rend, SDL_Rect &r, Sample v_min, Sample v_max)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	for(int n=6; n>=-6; n--) {
		Sample base = pow(10.0f, n);
		float dy = (base / (v_max - v_min)) * r.h;
		if(dy < 10) break;
		int col = std::clamp((int)(dy-10) * 4, 0, 64);
		SDL_SetRenderDrawColor(rend, col, col, col, 255);
		Sample v_start = ceilf(v_min / base) * base;
		Sample v_end   = floorf(v_max / base) * base;
		Sample v = v_start;
		while(v <= v_end) {
			int y = r.y + r.h - (int)((v - v_min) / (v_max - v_min) * r.h);
			SDL_RenderLine(rend, r.x, y, r.x + r.w, y);
			if(dy > 20) {
				char buf[32];
				snprintf(buf, sizeof(buf), "%.0f", v);
				dl->AddText(ImVec2(r.x + 2, y), 0xFF808080, buf);
			}
			v += base;
		}
	}

}




SDL_Color Widget::channel_color(int channel)
{
	// https://medialab.github.io/iwanthue/
	static const SDL_Color color[] = {
		{  82, 202,  59, 255 },
		{ 222,  85, 231, 255 },
		{  68, 124, 254, 255 },
		{ 244,  74,  54, 255 },
		{ 175, 198,  31, 255 },
		{ 163, 102, 248, 255 },
		{ 242,  62, 173, 255 },
		{ 239, 157,  22, 255 },
	};
	if(channel >= 0 && channel < 8) {
		return color[channel];
	} else {
		return SDL_Color{ 255, 255, 255, 255 };
	}
}


const char *k_type_str[] = {
	"none", "wave", "spectrum", "waterfall", "histogram", "style editor"
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


namespace ImGui {
bool ToggleButton(const char* str_id, bool* v)
{
	ImVec4 col = *v ? ImVec4(0.26f, 0.59f, 0.98f, 1.0f) : ImVec4(0.26f, 0.26f, 0.38f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, col);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
	bool pressed = ImGui::Button(str_id);
	if(pressed) *v = !*v;
	ImGui::PopStyleColor(3);
	return pressed;
}
}


