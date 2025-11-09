
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <generator>

#include "misc.hpp"
#include "widget.hpp"
#include "widgetregistry.hpp"


Widget::Widget(Widget::Info &info)
	: m_info(info)
{
}


Widget::~Widget()
{
}


void Widget::load(ConfigReader::Node *node)
{
	m_channel_map.load(node);
	m_view.load(node);
	do_load(node);
}


void Widget::save(ConfigWriter &cw)
{
	cw.write("widget", m_info.name);
	m_view.save(cw);
	m_channel_map.save(cw);
	do_save(cw);
}


Widget *Widget::copy()
{
	Widget *w_new = Widgets::create_widget(m_info.name);
	do_copy(w_new);
	copy_to(w_new);
	return w_new;
}


void Widget::copy_to(Widget *w)
{
	w->m_view = m_view;
	w->m_channel_map = m_channel_map;
}


void Widget::draw(View &view, Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	if(m_view.lock) m_view = view;
	m_view.time.cursor = view.time.cursor;
	m_view.time.playpos = view.time.playpos;

	if(m_info.flags & Widget::Info::Flags::ShowChannelMap) {
		m_channel_map.set_channel_count(stream.channel_count());
		m_channel_map.draw();
	}

	if(m_info.flags & Widget::Info::Flags::ShowLock) {
		ImGui::SameLine();
		ImGui::ToggleButton("L##ock", &m_view.lock);
		// key 'L': toggle lock
		if(ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_L)) {
			m_view.lock = !m_view.lock;
		}
	}

	if(m_info.flags & Widget::Info::Flags::ShowWindowSize) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderInt("##fft size", (int *)&m_view.window.size, 
					16, 32768, "%d", ImGuiSliderFlags_Logarithmic);
	}

	if(m_info.flags & Widget::Info::Flags::ShowWindowType) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::Combo("##window", (int *)&m_view.window.window_type, 
				Window::type_names(), Window::type_count());
		if(m_view.window.window_type == Window::Type::Gauss || 
		   m_view.window.window_type == Window::Type::Kaiser) {
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100);
			float beta = m_view.window.window_beta;
			ImGui::SliderFloat("beta", &beta,
					0.0f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
			m_view.window.window_beta = beta;
		}
	}
	

	ImGui::SameLine();
	
	if(ImGui::IsWindowFocused()) {

		// key '[': decrease window size
		if(ImGui::IsKeyPressed(ImGuiKey_LeftBracket)) {
			for(size_t i=30; i>1; i--) {
				int s = 1<<i;
				if(s < m_view.window.size) {
					m_view.window.size = s;
					break;
				}
			}
		}
		// key ']': increase window size
		if(ImGui::IsKeyPressed(ImGuiKey_RightBracket)) {
			for(size_t i=1; i<30; i++) {
				int s = 1<<i;
				if(s > m_view.window.size) {
					m_view.window.size = s;
					break;
				}
			}
		}

		// key 'left': pan time
		bool panning = false;
		if(ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
			m_view.pan_t(+m_pan_speed);
			panning = true;
		}
		// key 'right': pan time
		if(ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
			m_view.pan_t(-m_pan_speed);
			panning = true;
		}

		// key 'up': zoom in time
		if(ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
			m_view.zoom_t(-m_pan_speed * 100, View::ZoomAnchor::Middle);
			panning = true;
		}
		// key 'down': zoom out time
		if(ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
			m_view.zoom_t(+m_pan_speed * 100, View::ZoomAnchor::Middle);
			panning = true;
		}
		m_pan_speed = panning ? m_pan_speed + 0.002 : 0.0;

		// mouse wheel scrolls time
		ImGuiIO& io = ImGui::GetIO();
		m_view.pan_t(io.MouseWheel * 0.1f);
	}

	// draw widget
	float t1 = hirestime();
	do_draw(stream, rend, r);
	float t2 = hirestime();

	// draw render time
	if(1) {
		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 60, ImGui::GetWindowHeight() - 25));
		ImGui::Text("%.2f ms", (t2 - t1) * 1000.0f);
	}
	
	if(m_view.lock) view = m_view;
	view.time.cursor = m_view.time.cursor;
	view.time.playpos = m_view.time.playpos;
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


void draw_float(int x, int y, float v)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	char buf[32];
	snprintf(buf, sizeof(buf), "%.1f", v);
	dl->AddText(ImVec2(x, y), 0xFF808080, buf);
}



static void grid_aux(SDL_Renderer *rend, SDL_Rect &r, float v_min, float v_max, bool hor)
{
	int p_min = hor ? r.x : r.y;
	int p_max = hor ? r.x + r.w : r.y + r.h;

	float subs[] = { 1.0, 5.0 };
	int ngrids = 0;
	for(int n=-6; n<=6; n++) {
		for(int s=0; s<2; s++) {
			float base = subs[s] * pow(10.0f, n);
			float dpx = fabs((base / (v_max - v_min)) * (p_max - p_min));
			if(dpx >= 20) {
				float v1 = ceilf(v_min / base) * base;
				float v2   = floorf(v_max / base) * base;
				float v_start = fmin(v1, v2);
				float v_end   = fmax(v1, v2);
				float v = v_start;
				int col = std::clamp((int)(dpx-20) * 4, 0, 16);
				SDL_SetRenderDrawColor(rend, col, col, col, 255);
				while(v <= v_end) {
					if(hor) {
						int x = r.x + (int)((v - v_min) / (v_max - v_min) * r.w);
						SDL_RenderLine(rend, x, r.y, x, r.y + r.h);
						if(dpx > 40) draw_float(x, r.y + 2, v);
					} else {
						int y = r.y + (int)((v - v_min) / (v_max - v_min) * r.h);
						SDL_RenderLine(rend, r.x, y, r.x + r.w, y);
						if(dpx > 40) draw_float(r.x + 2, y, v);
					}
					v += base;
				}
				if(ngrids++ >= 1) return;
			}

		}
	}
}


void Widget::grid_vertical(SDL_Renderer *rend, SDL_Rect &r, float v_min, float v_max)
{
	grid_aux(rend, r, v_max, v_min, false);
}


void Widget::grid_horizontal(SDL_Renderer *rend, SDL_Rect &r, float v_min, float v_max)
{
	grid_aux(rend, r, v_min, v_max, true);
}

static void cursor(SDL_Renderer *rend, float x0, float y0, float x1, float y1, float aw, SDL_FColor col, bool flip)
{
	float ah = 6;
	SDL_Vertex v[8];
	v[0].position.x = x0 - aw;        v[0].position.y = y0;
	v[1].position.x = x1 + aw;        v[1].position.y = y0;
	v[2].position.x = x0;             v[2].position.y = y0 + ah;
	v[3].position.x = x1;             v[3].position.y = y0 + ah;
	v[4].position.x = x0;             v[4].position.y = y1 - ah;
	v[5].position.x = x1;             v[5].position.y = y1 - ah;
	v[6].position.x = x0 - aw;        v[6].position.y = y1;
	v[7].position.x = x1 + aw;        v[7].position.y = y1;
	if(flip) {
		for(int i=0; i<8; i++) {
			float tmp = v[i].position.y;
			v[i].position.y = v[i].position.x;
			v[i].position.x = tmp;
		}
	}
	for(int i=0; i<8; i++) v[i].color = col;
	int indices[] = { 0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 6, 5, 6, 7 };
	SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, col.a);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_RenderGeometry(rend, nullptr, v, 8, indices, 18);
}


void Widget::vcursor(SDL_Renderer *rend, SDL_Rect &r, int y, bool playpos)
{
	float aw = playpos ? 4.0f : 0.0f;
	SDL_FColor col = {192, 192, 192, 255};
	if(playpos) col = {0, 96, 128, 255};
	cursor(rend, y-1, r.x, y+2, r.x + r.w, aw, {0, 0, 0, 255}, true);
	cursor(rend, y, r.x, y+1, r.x + r.w, aw, col, true);
}

void Widget::hcursor(SDL_Renderer *rend, SDL_Rect &r, int x, bool playpos)
{
	float aw = playpos ? 4.0f : 0.0f;
	SDL_FColor col = {192, 192, 192, 255};
	if(playpos) col = {0, 96, 128, 255};
	cursor(rend, x-1, r.y, x+2, r.y + r.h, aw, {0, 0, 0, 255}, false);
	cursor(rend, x, r.y, x+1, r.y + r.h, aw, col, false);
}

