
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "histogram.hpp"
#include "widgetregistry.hpp"



class WidgetHistogram : public Widget {

public:
	WidgetHistogram(Widget::Info &info);
	~WidgetHistogram() override;

private:
	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	bool m_agc{true};
	int m_nbins{64};
	Sample m_vmin{};
	Sample m_vmax{};
	std::array<Histogram, 8> m_hists{};
};


WidgetHistogram::WidgetHistogram(Widget::Info &info)
	: Widget(info)
{
}


WidgetHistogram::~WidgetHistogram()
{
}


void WidgetHistogram::do_load(ConfigReader::Node *node)
{
	auto *wnode = node->find("waveform");
	wnode->read("agc", m_agc);
	wnode->read("nbins", m_nbins);
}


void WidgetHistogram::do_save(ConfigWriter &cw)
{
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.write("nbins", m_nbins);
	cw.pop();
}


void WidgetHistogram::do_copy(Widget *w)
{
	WidgetHistogram *wh = dynamic_cast<WidgetHistogram*>(w);
	wh->m_agc = m_agc;
	wh->m_nbins = m_nbins;
}


void WidgetHistogram::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::ToggleButton("AGC", &m_agc);
	
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::SliderInt("##nbins", &m_nbins, 16, 1024, "%d", ImGuiSliderFlags_Logarithmic);
		
	size_t frames_stride;
	size_t frames_avail;
	Sample *frames_data = stream.peek(&frames_stride, &frames_avail);

	for(auto & h : m_hists) {
		h.set_range(m_vmin, m_vmax);
		h.set_nbins(m_nbins);
		h.clear();
	}
	
	Sample vmin = frames_data[0];
	Sample vmax = frames_data[0];

	int idx_cursor = m_view.time.cursor * stream.sample_rate();
	int idx_from = std::max(idx_cursor - m_view.window.size * 0.5, 0.0);
	int idx_to   = std::min(idx_cursor + m_view.window.size * 0.5, (double)frames_avail);

	for(int idx=idx_from; idx<idx_to; idx++) {
		for(int ch : m_channel_map.enabled_channels()) {
			Sample v = frames_data[idx * frames_stride + ch];
			m_hists[ch].add(v);
			vmin = std::min(vmin, v);
			vmax = std::max(vmax, v);
		}
	}

	m_vmin = m_agc ? vmin : -k_sample_max;
	m_vmax = m_agc ? vmax : +k_sample_max;

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(int ch : m_channel_map.enabled_channels()) {
		SDL_Color col = m_channel_map.ch_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);
		auto b = m_hists[ch].bins();
		graph(rend, r,
				b.data(), b.size(), 1,
				0, m_nbins-1,
				(size_t)0, m_hists[ch].get_peak());
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
}


REGISTER_WIDGET(WidgetHistogram,
	.name = "histogram",
	.description = "Value histogram",
	.hotkey = ImGuiKey_F4,
	.flags = Widget::Info::Flags::ShowChannelMap | 
	         Widget::Info::Flags::ShowLock |
	         Widget::Info::Flags::ShowWindowSize,
);

