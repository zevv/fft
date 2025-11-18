
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetregistry.hpp"
#include "fft.hpp"



class WidgetSpectrum : public Widget {
public:
	WidgetSpectrum(Widget::Info &info);
	~WidgetSpectrum() override;

private:
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	float m_amp_cursor{0.0};
	Fft m_fft{};

	std::vector<Sample> m_out_graph;
};



WidgetSpectrum::WidgetSpectrum(Widget::Info &info)
	: Widget(info)
{
	m_view_config.x = View::Axis::Frequency;
	m_view_config.y = View::Axis::Amplitude;
}


WidgetSpectrum::~WidgetSpectrum()
{
}


void WidgetSpectrum::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	if(ImGui::IsWindowFocused()) {
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("f=%.6gHz amp=%.2fdB", m_view.freq.cursor * stream.sample_rate() * 0.5, m_amp_cursor);
	}

	m_fft.configure(m_view.window.size, m_view.window.window_type, m_view.window.window_beta);

	if(ImGui::IsKeyPressed(ImGuiKey_A)) {
		m_view.amplitude.from = -100.0;
		m_view.amplitude.to = 0.0;
	}

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(int ch : m_channel_map.enabled_channels()) {
		size_t stride = 0;
		size_t avail = 0;
		Sample *data = stream.peek(&stride, &avail);
		int idx = ((int)(stream.sample_rate() * m_view.time.analysis - m_view.window.size * 0.5)) * stride + ch;

		if(idx < 0) continue;
		if(idx >= (int)(avail * stride)) continue;

		auto out_graph = m_fft.run(&data[idx], stride);

		size_t npoints = m_view.window.size / 2 + 1;
		SDL_Color col = m_channel_map.ch_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);
		graph(rend, r,
				out_graph.data(), out_graph.size(), 1,
				m_view.freq.from * npoints, m_view.freq.to * npoints,
				m_view.amplitude.from, m_view.amplitude.to);
	}
	
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetSpectrum,
	.name = "spectrum",
	.description = "FFT spectrum graph",
	.hotkey = ImGuiKey_F2,
	.flags = Widget::Info::Flags::ShowChannelMap | 
	         Widget::Info::Flags::ShowLock |
			 Widget::Info::Flags::ShowWindowSize |
			 Widget::Info::Flags::ShowWindowType,
);

