
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
		wnode->read("nbins", m_nbins);
	}
}


void WidgetHistogram::save(ConfigWriter &cw)
{
	Widget::save(cw);
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.write("nbins", m_nbins);
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
				16, 1024, "%d", ImGuiSliderFlags_Logarithmic);

		
	size_t frames_stride;
	size_t frames_avail;
	Sample *frames_data = streams.peek(&frames_stride, &frames_avail);

	int idx_from = std::max(m_view.x_to_t(r.x,       r) * view.srate, 0.0);
	int idx_to   = std::min(m_view.x_to_t(r.x + r.w, r) * view.srate, (double)frames_avail);

	std::vector<std::vector<int>> bins;
	bins.resize(m_nbins);
	for(auto &b : bins) {
		b.assign(m_nbins, 0.0f);
	}
		
	Sample vmin = m_agc ? frames_data[0] : -1.0;
	Sample vmax = m_agc ? frames_data[0] :  1.0;

	if(m_vmax == m_vmin) m_vmax += 1;

	int bin_max = 0.0f;
	for(int idx=idx_from; idx<idx_to; idx++) {
		for(int ch=0; ch<8; ch++) {
			if(!m_channel_map[ch]) continue;

			Sample v = frames_data[idx * frames_stride + ch];
			vmin = std::min(vmin, v);
			vmax = std::max(vmax, v);

			int bin = (m_nbins - 1) * (v - m_vmin) / (m_vmax - m_vmin);
			bin = std::clamp(bin, 0, m_nbins - 1);
			bins[ch][bin] ++;
			bin_max = std::max(bin_max, bins[ch][bin]);
		}
	}

	m_vmin = m_agc ? vmin : -k_sample_max;
	m_vmax = m_agc ? vmax : +k_sample_max;

	// draw channel histograms

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(int ch=0; ch<8; ch++) {
		if(!m_channel_map[ch]) continue;

		SDL_Color col = channel_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);

		graph(rend, r,
				bins[ch].data(), m_nbins, 1,
				0, m_nbins-1,
				0, (int)bin_max);

	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
}
