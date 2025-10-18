#pragma once

#include <fftw3.h>
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

	float graph(View &view, SDL_Renderer *rend, SDL_Rect &r, 
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
		bool m_agc;
		float m_peak;
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
