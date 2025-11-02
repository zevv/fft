
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetinfo.hpp"
#include "widget-xy.hpp"


WidgetXY::WidgetXY()
	: Widget("xy")
{
}


WidgetXY::~WidgetXY()
{
	if(m_tex) SDL_DestroyTexture(m_tex);
}


void WidgetXY::do_load(ConfigReader::Node *node)
{
	auto *wnode = node->find("xy");
	wnode->read("decay", m_decay);
}


void WidgetXY::do_save(ConfigWriter &cw)
{
	cw.push("xy");
	cw.write("decay", m_decay);
	cw.pop();
}


void WidgetXY::do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::SliderFloat("Decay", &m_decay, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_Logarithmic);

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

	int cx = r.w / 2;
	int cy = r.h / 2;
	double sx = (double)(r.w / 2) / m_peak;
	double sy = (double)(r.h / 2) / m_peak;
	m_peak *= 0.99;
		
	for(int idx=idx_from; idx<idx_to; idx++) {
		Sample vx = frames_data[idx * frames_stride + ch_x];
		Sample vy = frames_data[idx * frames_stride + ch_y];
		m_peak = std::max(m_peak, (double)fabs(vx));
		m_peak = std::max(m_peak, (double)fabs(vy));
		point[npoints].x = cx + vx * sx;
		point[npoints].y = cy - vy * sy;
		npoints++;
	}
	
	SDL_Color col = m_channel_map.ch_color(ch_x);
	auto rt_prev = SDL_GetRenderTarget(rend);

	// phosphor decay
	SDL_SetRenderTarget(rend, m_tex);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(rend, 0, 0, 0, m_decay * 255);
	SDL_RenderFillRect(rend, nullptr);

	// draw new points
	SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 128);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	SDL_RenderLines(rend, point.data(), npoints);
	SDL_SetRenderTarget(rend, rt_prev);

	SDL_FRect dstf = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};
	SDL_RenderTexture(rend, m_tex, nullptr, &dstf);

}


REGISTER_WIDGET(WidgetXY,
	.name = "xy",
	.description = "X/Y constallation display",
	.hotkey = ImGuiKey_F6,
);

