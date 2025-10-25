
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
	w->m_peak = m_peak;
	return w;
}


void WidgetHistogram::do_draw(View &view, Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::Checkbox("AGC", &m_agc);

	int nbins = 256;
	std::vector<Sample> bins(nbins);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;
		size_t stride;
		size_t avail;
		Sample *data = streams.peek(ch, &stride, &avail);

		float idx_from = m_view.x_to_t(r.x,       r) * view.srate;
		float idx_to   = m_view.x_to_t(r.x + r.w, r) * view.srate;

		idx_from = std::max<float>(0.0f, idx_from);
		idx_to   = std::min<float>(avail, idx_to);

		bins.assign(nbins, 0.0f);
		float bin_max = 0.0;
		for(int idx=idx_from; idx<idx_to; idx++) {
			float v = data[idx * stride];
			int bin = (int)((v + 1.0f) * 0.5f * nbins);
			bin = std::clamp(bin, 0, nbins - 1);
			bins[bin] += 1.0f;
			bin_max = std::max(bin_max, bins[bin]);
		}

		SDL_Color col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);

		float peak = graph(rend, r,
				bins.data(), nbins, 1,
				0, nbins,
				0.0, bin_max);

		if(peak > m_peak) {
			m_peak = peak;
		}
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	m_peak *= 0.9f;
}
