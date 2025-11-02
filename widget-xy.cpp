
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget-xy.hpp"


WidgetXY::WidgetXY()
	: Widget(Widget::Type::XY)
{
}


WidgetXY::~WidgetXY()
{
	if(m_tex) SDL_DestroyTexture(m_tex);
}


void WidgetXY::do_load(ConfigReader::Node *node)
{
}


void WidgetXY::do_save(ConfigWriter &cw)
{
}


Widget *WidgetXY::do_copy()
{
	WidgetXY *w = new WidgetXY();
	return w;
}


void WidgetXY::do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	int ch_x = -1;
	int ch_y = -1;

	if(!m_tex || m_tex->w != r.w || m_tex->h != r.h) {
		if(m_tex) SDL_DestroyTexture(m_tex);
		m_tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32,
				SDL_TEXTUREACCESS_TARGET, r.w, r.h);
		auto rt_prev = SDL_GetRenderTarget(rend);
		SDL_SetRenderTarget(rend, m_tex);
		SDL_SetTextureBlendMode(m_tex, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
		SDL_RenderClear(rend);
		SDL_SetRenderTarget(rend, rt_prev);
	}

	for(auto ch : m_channel_map.enabled_channels()) 
		if(ch_x == -1) ch_x = ch; else if(ch_y == -1) ch_y = ch;
	if(ch_x == -1 || ch_y == -1) return;
	
	size_t frames_stride;
	size_t frames_avail;
	Sample *frames_data = streams.peek(&frames_stride, &frames_avail);

	int idx_from = m_view.time.playpos * m_view.srate;
	int idx_to   = idx_from + m_view.fft.size;

	if(idx_to < 0) return;
	if(idx_from > frames_avail) return;

	std::vector<SDL_FPoint> point(idx_to - idx_from);
	size_t npoints = 0;

	int cx = r.x + r.w / 2;
	int cy = r.y + r.h / 2;
	double scale = (double)(r.w / 2) / k_sample_max;
		
	for(int idx=idx_from; idx<idx_to; idx++) {
		Sample vx = frames_data[idx * frames_stride + ch_x];
		Sample vy = frames_data[idx * frames_stride + ch_y];
		point[npoints].x = cx + vx * scale;
		point[npoints].y = cy - vy * scale;
		npoints++;
	}

	
	SDL_Color col = m_channel_map.ch_color(ch_x);
	auto rt_prev = SDL_GetRenderTarget(rend);

	// phosphor decay
	SDL_SetRenderTarget(rend, m_tex);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 16);
	SDL_RenderFillRect(rend, nullptr);

	// draw new points
	SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 128);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	SDL_RenderLines(rend, point.data(), npoints);
	SDL_SetRenderTarget(rend, rt_prev);

	SDL_FRect dstf = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};
	SDL_RenderTexture(rend, m_tex, nullptr, &dstf);

}
