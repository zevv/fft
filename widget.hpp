
#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "types.hpp"
#include "view.hpp"
#include "stream.hpp"
#include "config.hpp"

class Flap;

class Widget {
public:
	
	enum class Type : int {
		None, Waveform, Spectrum, Waterfall, Histogram, StyleEditor
	};

	Widget(Type type);
	virtual ~Widget();
	
	virtual void load(ConfigReader::Node *node);
	virtual void save(ConfigWriter &cfg);
	Widget *copy();
	void copy_to(Widget *w);
	Type get_type() { return m_type; }
	bool has_focus() { return m_has_focus; }
	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

	static Widget *create(Widget::Type type);
	static Widget *create(const char *type_str);
	static const char* type_to_string(Type type);
	static Type string_to_type(const char *str);
	static const char **type_names();
	static size_t type_count();
	static SDL_Color channel_color(int channel);
	

protected:

	virtual Widget *do_copy() = 0;
	virtual void do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r) = 0;

	template<typename T>
	T graph(SDL_Renderer *rend, SDL_Rect &r,
						 T data[], size_t data_count, size_t stride,
						 double idx_from, double idx_to,
						 T y_min, T y_max);
	
	template<typename T>
	T graph(SDL_Renderer *rend, SDL_Rect &r,
						 T data_min[], T data_max[], size_t data_count, size_t stride,
						 double idx_from, double idx_to,
						 T y_min, T y_max);

	void grid_vertical(SDL_Renderer *rend, SDL_Rect &r, Sample v_from, Sample v_to);
	void grid_time(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);
	void grid_time_v(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);
	

	Widget::Type m_type{Widget::Type::None};
	bool m_channel_map[8]{true, true, true, true, true, true, true, true};
	bool m_lock_view{true};
	bool m_has_focus{false};

	View m_view{};
};


class WidgetNone : public Widget {
public:
	WidgetNone() : Widget(Widget::Type::None) {}
	void do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r) { }
	Widget *do_copy() { return new WidgetNone(); }
};


class WidgetStyleEditor : public Widget {
public:
	WidgetStyleEditor() : Widget(Widget::Type::StyleEditor) {}
	void do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r) { ImGui::ShowStyleEditor(); }
	Widget *do_copy() { return new WidgetStyleEditor(); }
};

namespace ImGui {
	bool ToggleButton(const char* str_id, bool* v);
}


