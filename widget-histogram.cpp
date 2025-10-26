
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget-histogram.hpp"


WidgetHistogram::WidgetHistogram()
	: Widget(Widget::Type::Histogram)
{
}


WidgetHistogram::~WidgetHistogram()
{
}


void WidgetHistogram::load(ConfigReader::Node *node)
{
	if(!node) return;
	Widget::load(node);
	if(auto *wnode = node->find("waveform")) {
		wnode->read("agc", m_agc);
	}
}


void WidgetHistogram::save(ConfigWriter &cw)
{
	Widget::save(cw);
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.pop();
}


Widget *WidgetHistogram::do_copy()
{
	WidgetHistogram *w = new WidgetHistogram();
	w->m_agc = m_agc;
	return w;
}


void WidgetHistogram::do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_agc);
	
	ImGui::SetNextItemWidth(100);
	ImGui::SameLine();
	ImGui::SliderInt("##nbins", &m_nbins, 
				16, 32768, "%d", ImGuiSliderFlags_Logarithmic);

	std::vector<Sample> bins(m_nbins);
		
	size_t stride;
	size_t avail;
	Sample *data = streams.peek(&stride, &avail);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;

		float idx_from = m_view.x_to_t(r.x,       r) * view.srate;
		float idx_to   = m_view.x_to_t(r.x + r.w, r) * view.srate;

		idx_from = std::max<float>(0.0f, idx_from);
		idx_to   = std::min<float>(avail, idx_to);
	
		Sample vmin = -1.0;
		Sample vmax =  1.0;
		if(m_agc) {
			for(int idx=idx_from; idx<idx_to; idx++) {
				Sample v = data[idx * stride + ch];
				if(idx == (int)idx_from || v < vmin) vmin = v;
				if(idx == (int)idx_from || v > vmax) vmax = v;
			}
		}

		bins.assign(m_nbins, 0.0f);
		float bin_max = 0.0;
		for(int idx=idx_from; idx<idx_to; idx++) {
			float v = data[idx * stride + ch];
			int bin = (v - vmin) / (vmax - vmin) * (m_nbins - 1);
			bin = std::clamp(bin, 0, m_nbins - 1);
			bins[bin] += 1.0f;
			bin_max = std::max(bin_max, bins[bin]);
		}

		SDL_Color col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);

		graph(rend, r,
				bins.data(), m_nbins, 1,
				0, m_nbins-1,
				0.0, bin_max);

	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
}
