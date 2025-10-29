
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
	m_channel_map.load(node);
	m_view.load(node);
	do_load(node);
}


void Widget::save(ConfigWriter &cw)
{
	cw.write("widget", Widget::type_to_string(m_type));
	m_view.save(cw);
	m_channel_map.save(cw);
	do_save(cw);
}


Widget *Widget::copy()
{
	Widget *w = do_copy();
	copy_to(w);
	return w;
}


void Widget::copy_to(Widget *w)
{
	w->m_view = m_view;
	w->m_channel_map = m_channel_map;
}


void Widget::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	m_has_focus = ImGui::IsWindowFocused();
	if(m_view.lock) m_view = view;

	m_channel_map.set_channel_count(streams.channel_count());
	m_channel_map.draw();

	ImGui::SameLine();
	ImGui::ToggleButton("L##ock", &m_view.lock);
	ImGui::SameLine();
	
	if(m_has_focus) {

		// 'L' toggles lock
		if(ImGui::IsKeyPressed(ImGuiKey_L)) {
			m_view.lock = !m_view.lock;
		}

		// left/right square bracked sets fft bin size to next power of two
		if(ImGui::IsKeyPressed(ImGuiKey_LeftBracket)) {
			for(size_t i=30; i>1; i--) {
				int s = 1<<i;
				if(s < m_view.fft.size) {
					m_view.fft.size = s;
					break;
				}
			}
		}
		if(ImGui::IsKeyPressed(ImGuiKey_RightBracket)) {
			for(size_t i=1; i<30; i++) {
				int s = 1<<i;
				if(s > m_view.fft.size) {
					m_view.fft.size = s;
					break;
				}
			}
		}

		// left/right arrow pan time, accelerating
		bool panning = false;
		if(ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
			m_view.pan_t(+m_pan_speed);
			panning = true;
		}
		if(ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
			m_view.pan_t(-m_pan_speed);
			panning = true;
		}

		// up/down keys zoom time, accelerating
		if(ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
			m_view.zoom_t(-m_pan_speed * 100);
			panning = true;
		}
		if(ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
			m_view.zoom_t(+m_pan_speed * 100);
			panning = true;
		}
		m_pan_speed = panning ? m_pan_speed + 0.002 : 0.0;

		// mouse wheel scrolls time
		ImGuiIO& io = ImGui::GetIO();
		m_view.pan_t(io.MouseWheel * 0.1f);
	}

	float t1 = SDL_GetPerformanceCounter();
	do_draw(streams, rend, r);
	float t2 = SDL_GetPerformanceCounter();

	// draw render time
	if(0) {
		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 60, ImGui::GetWindowHeight() - 25));
		ImGui::Text("%.2f ms", (t2 - t1) * 1000.0f / SDL_GetPerformanceFrequency());
	}
	
	if(m_view.lock) view = m_view;
}


#ifndef SAMPLE_FLOAT
template float  Widget::graph<float >(SDL_Renderer*, SDL_Rect&, float [],           size_t, size_t, double , double , float , float );
template float  Widget::graph<float >(SDL_Renderer*, SDL_Rect&, float [], float [], size_t, size_t, double , double , float , float );
#endif

template Sample Widget::graph<Sample>(SDL_Renderer*, SDL_Rect&, Sample[],           size_t, size_t, double, double, Sample, Sample);
template Sample Widget::graph<Sample>(SDL_Renderer*, SDL_Rect&, Sample[], Sample[], size_t, size_t, double, double, Sample, Sample);

template int    Widget::graph<int   >(SDL_Renderer*, SDL_Rect&, int   [],           size_t, size_t, double, double, int   , int   );
template int    Widget::graph<int   >(SDL_Renderer*, SDL_Rect&, int   [], int   [], size_t, size_t, double, double, int   , int   );

template int8_t Widget::graph<int8_t>(SDL_Renderer*, SDL_Rect&, int8_t[],           size_t, size_t, double, double, int8_t, int8_t);
template int8_t Widget::graph<int8_t>(SDL_Renderer*, SDL_Rect&, int8_t[], int8_t[], size_t, size_t, double, double, int8_t, int8_t);

template size_t Widget::graph<size_t>(SDL_Renderer*, SDL_Rect&, size_t[],           size_t, size_t, double, double, size_t, size_t);
template size_t Widget::graph<size_t>(SDL_Renderer*, SDL_Rect&, size_t[], size_t[], size_t, size_t, double, double, size_t, size_t);

template double Widget::graph<double>(SDL_Renderer*, SDL_Rect&, double[],           size_t, size_t, double, double, double, double);
template double Widget::graph<double>(SDL_Renderer*, SDL_Rect&, double[], double[], size_t, size_t, double, double, double, double);



template<typename T>
T Widget::graph(SDL_Renderer *rend, SDL_Rect &r,
					 T data[], size_t data_count, size_t stride,
					 double idx_from, double idx_to,
					 T y_min, T y_max)
{
	return graph(rend, r, data, data, data_count, stride,
		  idx_from, idx_to,
		  y_min, y_max);
}

template<typename T>
T Widget::graph(SDL_Renderer *rend, SDL_Rect &r,
					 T data_min[], T data_max[], size_t data_count, size_t stride,
					 double idx_from, double idx_to,
					 T y_min, T y_max)
{
	float y_scale = (r.h - 2) / ((double)y_min - (float)y_max);
	float y_off = r.y - (float)y_max * (float)y_scale;
	T v_peak = 0;

	int npoints = 0;
	int nrects = 0;
	int step = 2;
	double idx_scale = (idx_to - idx_from) / r.w;
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


void Widget::grid_vertical(SDL_Renderer *rend, SDL_Rect &r, float v_min, float v_max)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	for(int n=6; n>=-6; n--) {
		float base = pow(10.0f, n);
		float dy = (base / (v_max - v_min)) * r.h;
		if(dy < 10) break;
		int col = std::clamp((int)(dy-10) * 4, 0, 64);
		SDL_SetRenderDrawColor(rend, col, col, col, 255);
		float v_start = ceilf(v_min / base) * base;
		float v_end   = floorf(v_max / base) * base;
		float v = v_start;
		while(v <= v_end) {
			int y = r.y + r.h - (int)((v - v_min) / (v_max - v_min) * r.h);
			SDL_RenderLine(rend, r.x, y, r.x + r.w, y);
			if(dy > 20) {
				char buf[32];
				snprintf(buf, sizeof(buf), "%d", (int)v);
				dl->AddText(ImVec2(r.x + 2, y), 0xFF808080, buf);
			}
			v += base;
		}
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
