
#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "types.hpp"
#include "view.hpp"
#include "stream.hpp"
#include "config.hpp"
#include "channelmap.hpp"
#include "widgetinfo.hpp"

class Widget {
public:
	
	Widget(WidgetInfo &info);
	virtual ~Widget();
	
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Widget *copy();
	void copy_to(Widget *w);

	const char *get_name() { return m_info.name; }
	bool has_focus() { return m_has_focus; }
	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);

protected:

	virtual void do_copy(Widget *w) {};
	virtual void do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r) {};
	virtual void do_load(ConfigReader::Node *node) {};
	virtual void do_save(ConfigWriter &cfg) {};

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

	void grid_vertical(SDL_Renderer *rend, SDL_Rect &r, float v_from, float v_to);
	void grid_time(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);
	void grid_time_v(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);
	
	WidgetInfo &m_info;
	bool m_has_focus{false};
	View m_view{};
	ChannelMap m_channel_map{};
	double m_pan_speed{};
};

