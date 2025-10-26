#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "fft.hpp"
#include "widget.hpp"

class Flap;

class WidgetWaterfall : public Widget {
public:
	WidgetWaterfall();
	~WidgetWaterfall() override;
	void load(ConfigReader::Node *node) override;
	void save(ConfigWriter &cfg) override;

private:
	Widget *do_copy() override;
	void do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r) override;

	void configure_fft(int size, Window::Type window_type);


	int m_size{256};
	float m_amp_cursor{0.0};
	Window::Type m_window_type{Window::Type::Hanning};
	float m_window_beta{5.0f};

	std::vector<Sample> m_in;
	std::vector<Sample> m_out_graph;
	Window m_window;
	Fft m_fft{};
	float m_peak{};
};
