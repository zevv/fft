
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "flap.hpp"
		


Flap::Flap(Type type)
	: m_type(type)
	, m_channel_map{}
{
	for(int i=0; i<8; i++) {
		m_channel_map[i] = true;
	}
}


Flap::~Flap()
{
}


void Flap::load(ConfigReader::Node *n)
{
	if(const char *type = n->read_str("type")) {
		m_type = Flap::string_to_type(type);
		int channel_map = 0;
		if(n->read("channel_map", channel_map)) {
			for(int i=0; i<8; i++) {
				m_channel_map[i] = (channel_map & (1 << i)) ? true : false;
			}
		}
		if(m_type == Type::Waveform) m_waveform.load(n->find("waveform"));
		if(m_type == Type::Spectrum) m_spectrum.load(n->find("spectrum"));
		if(m_type == Type::Waterfall) m_waterfall.load(n->find("waterfall"));
	}
}


void Flap::save(ConfigWriter &cw)
{
	cw.push("widget");

	int channel_map = 0;
	for(int i=0; i<8; i++) {
		if(m_channel_map[i]) channel_map |= (1 << i);
	}

	cw.write("type", Flap::type_to_string(m_type));
	cw.write("channel_map", channel_map);

	if(m_type == Type::Waveform) m_waveform.save(cw);
	if(m_type == Type::Spectrum) m_spectrum.save(cw);
	if(m_type == Type::Waterfall) m_waterfall.save(cw);

	cw.pop();

}


Flap *Flap::copy(void)
{
	Flap *w = new Flap(m_type);
	for(int i=0; i<8; i++) {
		w->m_channel_map[i] = m_channel_map[i];
	}
	m_waveform.copy_to(w->m_waveform);
	m_spectrum.copy_to(w->m_spectrum);
	m_waterfall.copy_to(w->m_waterfall);
	return w;
}



void Flap::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
{
	m_has_focus = ImGui::IsWindowFocused();

	ImGui::SetNextItemWidth(100);
	ImGui::Combo("type", (int *)&m_type, 
			Flap::type_names(), Flap::type_count());

	// Channel enable buttons
	for(size_t i=0; i<8; i++) {
		ImGui::SameLine();
		SDL_Color c = channel_color(i);
		ImVec4 col = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		if(channel_enabled(i)) col = ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
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

	ImGui::SameLine();
	ImGui::Checkbox("L##lock view", &m_lock_view);

	if(ImGui::IsWindowFocused()) {
		if(ImGui::IsKeyPressed(ImGuiKey_0)) {
			for(int i=0; i<8; i++) m_channel_map[i] ^= 1;
		}
		if(ImGui::IsKeyPressed(ImGuiKey_F1)) m_type = Type::Waveform;
		if(ImGui::IsKeyPressed(ImGuiKey_F2)) m_type = Type::Spectrum;
		if(ImGui::IsKeyPressed(ImGuiKey_F3)) m_type = Type::Waterfall;
		if(ImGui::IsKeyPressed(ImGuiKey_L)) m_lock_view = !m_lock_view;
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

	if(m_type == Type::Waveform) m_waveform.draw(*this, view, streams, rend, r);
	if(m_type == Type::Spectrum) m_spectrum.draw(*this, view, streams, rend, r);
	if(m_type == Type::Waterfall) m_waterfall.draw(*this, view, streams, rend, r);
	

	SDL_SetRenderClipRect(rend, nullptr);
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

void Flap::grid_time(SDL_Renderer *rend, SDL_Rect &r, Time t_min, Time t_max)
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

void Flap::grid_time_v(SDL_Renderer *rend, SDL_Rect &r, Time t_min, Time t_max)
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


void Flap::grid_vertical(SDL_Renderer *rend, SDL_Rect &r, Sample v_min, Sample v_max)
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



Sample Flap::graph(SDL_Renderer *rend, SDL_Rect &r,
					 Sample *data, size_t data_count, size_t stride,
					 float idx_from, float idx_to,
					 Sample y_min, Sample y_max)
{
	float y_scale = (r.h - 2) / ((float)y_min - (float)y_max);
	float y_off = r.y - (float)y_max * (float)y_scale;
	Sample v_peak = 0;

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

		Sample vmin = data[stride * idx_start];
		Sample vmax = data[stride * idx_start];

		for(int idx=idx_start; idx<idx_end; idx++) {
			Sample v = 0;
			if(idx >= idx_min && idx < idx_max) {
				v = data[stride * idx];
			}
			if(v < vmin) vmin = v;
			if(v > vmax) vmax = v;
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


SDL_Color Flap::channel_color(int channel)
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
	"none", "wave", "spectrum", "waterfall"
};

const char **Flap::type_names()
{
	return k_type_str;
}

size_t Flap::type_count()
{
	return IM_ARRAYSIZE(k_type_str);
}

const char *Flap::type_to_string(Type type)
{
	size_t count = type_count();
	const char **names = type_names();
	for(size_t i=0; i<count; i++) {
		if((Type)i == type) return names[i];
	}
	return "none";
}


Flap::Type Flap::string_to_type(const char *str)
{
	size_t count = type_count();
	const char **names = type_names();
	for(size_t i=0; i<count; i++) {
		if(strcmp(names[i], str) == 0) return (Type)i;
	}
	return Type::None;
}
