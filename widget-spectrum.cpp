
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetregistry.hpp"
#include "fft.hpp"
#include "style.hpp"



class WidgetSpectrum : public Widget {
public:
	WidgetSpectrum(Widget::Info &info);
	~WidgetSpectrum() override;
	
	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;

private:
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	Fft m_fft{};
	Fft::Mode m_mode{Fft::Mode::Log};

	std::vector<Sample> m_out_graph;
};



WidgetSpectrum::WidgetSpectrum(Widget::Info &info)
	: Widget(info)
{
	m_view_config.x = View::Axis::Frequency;
	m_view_config.y = View::Axis::Aperture;
}


WidgetSpectrum::~WidgetSpectrum()
{
}


void WidgetSpectrum::do_load(ConfigReader::Node *node)
{
	int mode;
	node->read("mode", mode);
	m_mode = static_cast<Fft::Mode>(mode);
}


void WidgetSpectrum::do_save(ConfigWriter &cw)
{
	cw.write("mode", (int)m_mode);
}


void WidgetSpectrum::do_copy(Widget *w)
{
	auto *ws = dynamic_cast<WidgetSpectrum *>(w);
	m_mode = ws->m_mode;
}


void WidgetSpectrum::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	bool log = m_mode == Fft::Mode::Log;
	ImGui::SameLine();
	ImGui::ToggleButton("LOG", &log);
	m_mode = log ? Fft::Mode::Log : Fft::Mode::Lin;

	if(ImGui::IsWindowFocused()) {
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("f=%.6gHz amp=%.2fdB", m_view.freq.cursor * stream.sample_rate() * 0.5, m_view.aperture.cursor);
	}

	m_view_config.y = (m_mode == Fft::Mode::Log) ? View::Axis::Aperture : View::Axis::Amplitude;
	double graph_min = (m_mode == Fft::Mode::Log) ? m_view.aperture.from : m_view.amplitude.from;
	double graph_max = (m_mode == Fft::Mode::Log) ? m_view.aperture.to : m_view.amplitude.to;

	m_fft.configure(m_view.window.size, m_view.window.window_type, m_view.window.window_beta, m_mode);

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
		SDL_SetRenderDrawColor(rend, Style::channel_color(ch));
		graph(rend, r,
				out_graph.data(), out_graph.size(), 1,
				1.00,
				m_view.freq.from * npoints, m_view.freq.to * npoints,
				graph_min, graph_max);
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

