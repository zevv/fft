
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
		None, Waveform, Spectrum, Waterfall
	};

	Widget(Type type);
	virtual ~Widget();
	
	virtual void load(ConfigReader::Node *node);
	virtual void save(ConfigWriter &cfg);
	Widget *copy();
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

	Sample graph(SDL_Renderer *rend, SDL_Rect &r,
						 Sample *data, size_t data_count, size_t stride,
						 float idx_from, float idx_to,
						 Sample y_min, Sample y_max);

	void grid_vertical(SDL_Renderer *rend, SDL_Rect &r, Sample v_from, Sample v_to);
	void grid_time(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);
	void grid_time_v(SDL_Renderer *rend, SDL_Rect &r, Time t_from, Time t_to);
	
	Time x_to_t(float x, SDL_Rect &r) {
		return m_t_from - (m_t_from - m_t_to) * (x - r.x) / r.w;
	}

	float t_to_x(Time t, SDL_Rect &r) {
		return r.x + r.w * (m_t_from - t) / (m_t_from - m_t_to);
	}

	Time dx_to_dt(float dx, SDL_Rect &r) {
		return dx * (m_t_to - m_t_from) / r.w;
	}

	
	Frequency x_to_freq(float x, SDL_Rect &r) {
		return m_freq_from + (m_freq_to - m_freq_from) * (x - r.x) / r.w;
	}

	float freq_to_x(Frequency freq, SDL_Rect &r) {
		return r.x + r.w * (freq - m_freq_from) / (m_freq_to - m_freq_from);
	}

	Frequency dx_to_dfreq(float dx, SDL_Rect &r) {
		return dx * (m_freq_to - m_freq_from) / r.w;
	}


	Time y_to_t(float y, SDL_Rect &r) {
		return m_t_from - (m_t_from - m_t_to) * (y - r.y) / r.h;
	}

	float t_to_y(Time t, SDL_Rect &r) {
		return r.y + r.h * (m_t_from - t) / (m_t_from - m_t_to);
	}

	Time dy_to_dt(float dy, SDL_Rect &r) {
		return dy * (m_t_to - m_t_from) / r.h;
	}

	///

	void pan_t(float dx, float max) {
		Time dt = -dx * (m_t_to - m_t_from) / max;
		m_t_from += dt;
		m_t_to += dt;
	};

	void zoom_t(float fy, float max) {
		fy /= 50.0;
		m_t_from += (m_t_cursor - m_t_from) * fy;
        m_t_to   -= (m_t_to - m_t_cursor) * fy;
	};
	
	void pan_freq(float dx, float max) {
		Frequency df = dx * (m_freq_to - m_freq_from) / max;
		m_freq_from += df;
		m_freq_to += df;
	};
	
	void zoom_freq(float fx, float max) {
		fx /= 50.0;
		m_freq_from += (m_freq_cursor - m_freq_from) * fx;
		m_freq_to   -= (m_freq_to - m_freq_cursor) * fx;
	};


	Widget::Type m_type{Widget::Type::None};
	bool m_channel_map[8]{true, true, true, true, true, true, true, true};
	bool m_lock_view{true};
	bool m_has_focus{false};

	Time m_t_from{};
	Time m_t_to{};
	Time m_t_cursor{};
	
	Frequency m_freq_from{0.0};
	Frequency m_freq_to{1.0};
	Frequency m_freq_cursor{0.0};
};

