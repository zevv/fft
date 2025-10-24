#pragma once

#include <SDL3/SDL.h>

#include "config.hpp"
#include "types.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "fft.hpp"
#include "widget.hpp"


class Spectrum : public Widget {
public:
	Spectrum();
	~Spectrum() override;
	void load(ConfigReader::Node *node);
	void save(ConfigWriter &cfg);
	void copy_to(Spectrum &w);
	void draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r);
private:
	void configure_fft(int size, Window::Type window_type);

	int m_size{256};
	float m_amp_cursor{0.0};
	Window::Type m_window_type{Window::Type::Hanning};
	float m_window_beta{5.0f};
	Fft m_fft{};

	std::vector<Sample> m_in;
	std::vector<Sample> m_out_graph;
};
