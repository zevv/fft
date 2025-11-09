
#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "types.hpp"
#include "view.hpp"
#include "stream.hpp"
#include "config.hpp"
#include "channelmap.hpp"

class Widget;

class Widget {
public:

	struct Info {

		enum Flags {
			ShowChannelMap	   = 1 << 0,
			ShowLock	       = 1 << 1,
			ShowWindowSize     = 1 << 2,
			ShowWindowType     = 1 << 3,
		};

		const char *name;
		const char *description;
		enum ImGuiKey hotkey;
		int flags;
		Widget *(*fn_new)();
	};


	
	Widget(Info &info);
	virtual ~Widget();
	
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Widget *copy();
	void copy_to(Widget *w);

	const char *get_name() { return m_info.name; }
	void draw(View &view, Stream &stream, SDL_Renderer *rend, SDL_Rect &r);

protected:

	virtual void do_copy(Widget *w) {};
	virtual void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) {};
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
	void grid_horizontal(SDL_Renderer *rend, SDL_Rect &r, float v_from, float v_to);
	void grid_time(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);
	void grid_time_v(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);

	enum class CursorType {
		HorWire,
		HorPlaypos,
		VerWire,
		VerPlaypos
	};

	void vcursor(SDL_Renderer *rend, SDL_Rect &r, int y, bool playpos);
	void hcursor(SDL_Renderer *rend, SDL_Rect &r, int x, bool playpos);

	Info &m_info;
	View m_view{};
	ChannelMap m_channel_map{};
	double m_pan_speed{};
};

