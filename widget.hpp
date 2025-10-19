#pragma once

#include <fftw3.h>
#include <imgui.h>

#include "stream.hpp"
#include "window.hpp"
#include "view.hpp"
#include "config.hpp"


class Widget {
	

public:
	enum Type : int {
		None, Waveform, Spectrum, Waterfall
	};

	Widget(Type type = None);
	~Widget();

	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	Widget *copy();


	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	void draw_spectrum(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	bool has_focus() const { return m_has_focus; }
	bool channel_enabled(int channel) const { return m_channel_map[channel]; }
	ImVec4 channel_color(int channel);

	static const char* type_to_string(Type type);
	static Type string_to_type(const char *str);
	static const char **type_names();
	static size_t type_count();

private:

	float graph(SDL_Renderer *rend, SDL_Rect &r, 
					 ImVec4 &col, float *data, size_t stride,
					 int idx_from, int idx_to,
					 float y_min, float y_max);
	

	Type m_type;
	bool m_channel_map[8];

	class Waveform {
	public:
		Waveform();
		void load(ConfigReader::Node *node);
		void save(ConfigWriter &cfg);
		void copy_to(Waveform &w);
		void draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	private:

		float x_to_idx(float x, SDL_Rect &r)
		{
			return m_idx_from - (m_idx_from - m_idx_to) * (x - r.x) / r.w;
		}

		float idx_to_x(float idx, SDL_Rect &r)
		{
			return r.x + r.w * (m_idx_from - idx) / (m_idx_from - m_idx_to);
		}

		float didx_to_dx(float didx, SDL_Rect &r)
		{
			return r.w * didx / (m_idx_from - m_idx_to);
		}

		float dx_to_didx(float dx, SDL_Rect &r)
		{
			return dx * (m_idx_to - m_idx_from) / r.w;
		}

		void pan(float delta) {
			m_idx_from += delta;
			m_idx_to += delta;
		};

		void zoom(float f)
		{
			m_idx_from += (m_idx_cursor - m_idx_from) * f;
			m_idx_to   -= (m_idx_to - m_idx_cursor) * f;
		};

		bool m_agc;
		float m_peak;
		float m_idx_from;
		float m_idx_to;
		float m_idx_cursor;
	} m_waveform;

	struct Spectrum {
	public:
		Spectrum();
		~Spectrum();
		void load(ConfigReader::Node *node);
		void save(ConfigWriter &cfg);
		void copy_to(Spectrum &w);
		void draw(Widget &widget, View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
	private:
		void configure_fft(int size, Window::Type window_type);

		int m_size;
		Window::Type m_window_type;
		float m_window_beta;
		std::vector<float> m_in;
		std::vector<float> m_out;
		fftwf_plan m_plan;
		Window m_window;
	} m_spectrum;

	bool m_has_focus;
};
