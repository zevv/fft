
#include <math.h>
#include <assert.h>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget.hpp"
		


Widget::Widget(Type type)
	: m_type(type)
	, m_channel_map{}
{
	for(int i=0; i<8; i++) {
		m_channel_map[i] = true;
	}
}


Widget::~Widget()
{
}


void Widget::load(ConfigReader::Node *n)
{
	if(const char *type = n->read_str("type")) {
		m_type = Widget::string_to_type(type);
		int channel_map = 0;
		if(n->read("channel_map", channel_map)) {
			for(int i=0; i<8; i++) {
				m_channel_map[i] = (channel_map & (1 << i)) ? true : false;
			}
		}
		if(m_type == Type::Waveform) m_waveform.load(n->find("waveform"));
		if(m_type == Type::Spectrum) m_spectrum.load(n->find("spectrum"));
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
		m_waveform.save(cw);
	}

	if(m_type == Type::Spectrum) {
		m_spectrum.save(cw);
	}

	cw.pop();

}


Widget *Widget::copy(void)
{
	Widget *w = new Widget(m_type);
	for(int i=0; i<8; i++) {
		w->m_channel_map[i] = m_channel_map[i];
	}
	m_waveform.copy_to(w->m_waveform);
	m_spectrum.copy_to(w->m_spectrum);
	return w;
}



void Widget::draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &_r)
{
	m_has_focus = ImGui::IsWindowFocused();

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
		m_spectrum.draw(*this, view, streams, rend, r);
	} else if(m_type == Type::Waterfall) {
		ImGui::ShowStyleEditor();
	} else if(m_type == Type::Waveform) {
		m_waveform.draw(*this, view, streams, rend, r);
	}

	SDL_SetRenderClipRect(rend, nullptr);
}


Sample Widget::graph(SDL_Renderer *rend, SDL_Rect &r, ImVec4 &col,
					 Sample *data, size_t stride,
					 float idx_from, float idx_to,
					 int idx_min, int idx_max,
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

	SDL_SetRenderDrawColor(rend, col.x * 255, col.y * 255, col.z * 255, col.w * 255);
	SDL_RenderLines(rend, p_max.data(), npoints);
	SDL_RenderLines(rend, p_min.data(), npoints);
	SDL_SetRenderDrawColor(rend, col.x * 48, col.y * 48, col.z * 48, col.w * 255);
	SDL_RenderFillRects(rend, rects.data(), nrects);

	return v_peak;
}


ImVec4 Widget::channel_color(int channel)
{
	// https://medialab.github.io/iwanthue/
	static const ImVec4 color[] = {
		{  82.0/255, 202.0/255,  59.0/255, 1.0f },
		{ 222.0/255,  85.0/255, 231.0/255, 1.0f },
		{  68.0/255, 124.0/255, 254.0/255, 1.0f },
		{ 244.0/255,  74.0/255,  54.0/255, 1.0f },
		{ 175.0/255, 198.0/255,  31.0/255, 1.0f },
		{ 163.0/255, 102.0/255, 248.0/255, 1.0f },
		{ 242.0/255,  62.0/255, 173.0/255, 1.0f },
		{ 239.0/255, 157.0/255,  22.0/255, 1.0f },
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

