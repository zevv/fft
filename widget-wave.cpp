
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "widgetregistry.hpp"


class WidgetWaveform : public Widget {

public:
	WidgetWaveform(Widget::Info &info);
	~WidgetWaveform() override;

private:
	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	bool m_agc{true};
	Sample m_peak{};
};



WidgetWaveform::WidgetWaveform(Widget::Info &info)
	: Widget(info)
{
	m_view_config.x = View::Axis::Time;
	m_view_config.y = View::Axis::Amplitude;
}


WidgetWaveform::~WidgetWaveform()
{
}


void WidgetWaveform::do_load(ConfigReader::Node *node)
{
	auto *wnode = node->find("waveform");
	wnode->read("agc", m_agc);
}


void WidgetWaveform::do_save(ConfigWriter &cw)
{
	cw.push("waveform");
	cw.write("agc", m_agc);
	cw.pop();
}


void WidgetWaveform::do_copy(Widget *w)
{
	WidgetWaveform *ww = dynamic_cast<WidgetWaveform*>(w);
	ww->m_agc = m_agc;
	ww->m_peak = m_peak;
}


void WidgetWaveform::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::ToggleButton("AGC", &m_agc);
	
	size_t frames_avail;
	size_t data_stride;
	Sample *data = stream.peek(&data_stride, &frames_avail);

	size_t wframes_avail;
	size_t wdata_stride;
	Wavecache::Range *wdata = stream.peek_wavecache(&wdata_stride, &wframes_avail);

	if(ImGui::IsWindowFocused()) {
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("t=%.4gs", m_view.time.cursor);
	}


	Sample scale = k_sample_max;
	if(m_agc && m_peak > 0.0f) {
		m_view.amplitude.from = -m_peak * 1.1f;
		m_view.amplitude.to   = +m_peak * 1.1f;
	}
	m_peak *= 0.9f;

	double idx_from = m_view.time.from * stream.sample_rate();
	double idx_to   = m_view.time.to   * stream.sample_rate();
	double step = (idx_to - idx_from) / r.w;

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(auto ch : m_channel_map.enabled_channels()) {

		SDL_Color col = m_channel_map.ch_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);

		Sample peak;

		if(step < 256) {
			peak = graph(rend, r,
					data + ch, frames_avail, data_stride,
					idx_from, idx_to,
					(Sample)m_view.amplitude.from, (Sample)m_view.amplitude.to);
		} else {
			peak = graph(rend, r,
					&wdata[ch].min, &wdata[ch].max, 
					frames_avail / 256, wdata_stride * 2,
					idx_from / 256, idx_to / 256,
					(Sample)m_view.amplitude.from, (Sample)m_view.amplitude.to);
		}

		m_peak = std::max(m_peak, peak);
	}
	
	// selection
	//if(m_view.time.sel_from != m_view.time.sel_to) {
	//	float sx_from = m_view.t_to_x(m_view.time.sel_from, r);
	//	float sx_to   = m_view.t_to_x(m_view.time.sel_to,   r);
	//	SDL_SetRenderDrawColor(rend, 128, 128, 255, 64);
	//	SDL_FRect sr = { sx_from, (float)r.y, sx_to - sx_from, (float)r.h };
	//	SDL_RenderFillRect(rend, &sr);
	//}

	// window
	Window w = Window(m_view.window.window_type, m_view.window.size, m_view.window.window_beta);
	SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
	double w_idx_from = (m_view.time.from - m_view.time.analysis) * stream.sample_rate() + m_view.window.size * 0.5;
	double w_idx_to   = (m_view.time.to   - m_view.time.analysis) * stream.sample_rate() + m_view.window.size * 0.5;
	graph(rend, r, w.data().data(), w.size(), 1, w_idx_from, w_idx_to, 0.0f, +1.0f);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetWaveform,
	.name = "waveform",
	.description = "Waveform display",
	.hotkey = ImGuiKey_F1,
	.flags = Widget::Info::Flags::ShowChannelMap | Widget::Info::Flags::ShowLock,
);

