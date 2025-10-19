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
					 float idx_from, float idx_to,
					 float idx_min, float idx_max,
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

		float x_to_idx(float x, SDL_Rect &r) {
			return m_idx_from - (m_idx_from - m_idx_to) * (x - r.x) / r.w;
		}

		float idx_to_x(float idx, SDL_Rect &r) {
			return r.x + r.w * (m_idx_from - idx) / (m_idx_from - m_idx_to);
		}

		float dx_to_didx(float dx, SDL_Rect &r) {
			return dx * (m_idx_to - m_idx_from) / r.w;
		}

		void pan(float delta) {
			m_idx_from += delta;
			m_idx_to += delta;
		};

		void zoom(float f) {
			m_idx_from += (m_idx_cursor - m_idx_from) * f;
			m_idx_to   -= (m_idx_to - m_idx_cursor) * f;
		};

		bool m_agc{true};
		float m_peak{0.0};
		float m_idx_from{0.0};
		float m_idx_to{0.0};
		float m_idx_cursor{0.0};
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

		float x_to_freq(float x, SDL_Rect &r) {
			return m_freq_from + (m_freq_to - m_freq_from) * (x - r.x) / r.w;
		}

		float freq_to_x(float freq, SDL_Rect &r) {
			return r.x + r.w * (freq - m_freq_from) / (m_freq_to - m_freq_from);
		}

		float dx_to_dfreq(float dx, SDL_Rect &r) {
			return dx * (m_freq_to - m_freq_from) / r.w;
		}

		void pan(float delta) {
			m_freq_from += delta;
			m_freq_to += delta;
		};

		void zoom(float f) {
			m_freq_from += (m_freq_cursor - m_freq_from) * f;
			m_freq_to   -= (m_freq_to - m_freq_cursor) * f;
		};

		int m_size{256};
		float m_freq_from{0.0};
		float m_freq_to{1.0};
		float m_freq_cursor{0.0};
		float m_amp_cursor{0.0};
		Window::Type m_window_type{Window::Type::Hanning};
		float m_window_beta{5.0f};
		fftwf_plan m_plan{nullptr};

		std::vector<float> m_in;
		std::vector<float> m_out;
		Window m_window;
	} m_spectrum;

	bool m_has_focus;
};
