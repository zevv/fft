
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "widgetregistry.hpp"
#include "style.hpp"


class WidgetWaveform : public Widget {

public:
	WidgetWaveform(Widget::Info &info);
	~WidgetWaveform() override;

private:
	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;
	bool do_handle_input(Stream &stream, SDL_Rect &r) override;

	bool m_agc{true};
	Sample m_peak{};

	std::vector<int> m_channel_offset;
	int m_handle_dragging{-1};
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

static void draw_channel_handle(SDL_Renderer *rend, SDL_Rect &r, int y, Style::Color col)
{
	SDL_Vertex v[5];

	v[0].position = { (float)r.x,         (float)(y - 5) };
	v[1].position = { (float)(r.x + 8),   (float)(y - 5) };
	v[2].position = { (float)(r.x + 12),  (float)(y)     };
	v[3].position = { (float)(r.x + 8),   (float)(y + 5) };
	v[4].position = { (float)r.x,         (float)(y + 5) };
	for(int i=0; i<5; i++) v[i].color = col;
	int indices[] = { 0, 1, 2, 0, 2, 4, 4, 2, 3 };
	SDL_RenderGeometry(rend, nullptr, v, 5, indices, 9);
}


void WidgetWaveform::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	m_channel_offset.resize(stream.channel_count());

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

	SDL_Rect rwave = r;
	rwave.x += 16;
	rwave.w -= 16;

	for(auto ch : m_channel_map.enabled_channels()) {

		Style::Color col = Style::channel_color(ch);

		int y = m_view.from_amplitude(m_view_config, r, m_channel_offset[ch]);
		draw_channel_handle(rend, r, y, col);
		SDL_SetRenderDrawColor(rend, col);

		Sample peak;

		double offset = m_channel_offset[ch];

		if(step < 256) {
			peak = graph(rend, rwave,
					data + ch, frames_avail, data_stride,
					idx_from, idx_to,
					m_view.amplitude.from - offset,
					m_view.amplitude.to - offset);
		} else {
			peak = graph(rend, rwave,
					&wdata[ch].min, &wdata[ch].max, 
					frames_avail / 256, wdata_stride * 2,
					idx_from / 256, idx_to / 256,
					m_view.amplitude.from - offset,
					m_view.amplitude.to - offset);
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
	SDL_SetRenderDrawColor(rend, Style::color(Style::ColorId::AnalysisWindow));
	double w_idx_from = (m_view.time.from - m_view.time.analysis) * stream.sample_rate() + m_view.window.size * 0.5;
	double w_idx_to   = (m_view.time.to   - m_view.time.analysis) * stream.sample_rate() + m_view.window.size * 0.5;
	graph(rend, r, w.data().data(), w.size(), 1, w_idx_from, w_idx_to, 0.0f, +1.0f);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


bool WidgetWaveform::do_handle_input(Stream &stream, SDL_Rect &r)
{
    auto pos = ImGui::GetIO().MousePos;
    auto delta = ImGui::GetIO().MouseDelta;

	if(ImGui::IsKeyPressed(ImGuiKey_A)) {
		m_channel_offset.clear();
	}

	if(m_handle_dragging == -1) {
		for(auto ch : m_channel_map.enabled_channels()) {
			int y = m_view.from_amplitude(m_view_config, r, m_channel_offset[ch]);
			ImVec2 p1 = ImVec2((float)r.x,         (float)(y - 5));
			ImVec2 p2 = ImVec2((float)(r.x + 12),  (float)(y + 5));
			if(ImGui::IsMouseHoveringRect(p1, p2)) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
				if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					printf("start dragging channel %d handle\n", ch);
					m_handle_dragging = ch;
					return true;
				}
			}
		}
	} else {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		if(ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			double delta_offset = delta.y * (m_view.amplitude.to - m_view.amplitude.from) / r.h;
			m_channel_offset[m_handle_dragging] -= delta_offset;
			return true;
		} else {
			printf("End dragging channel %d handle\n", m_handle_dragging);
			m_handle_dragging = -1;
		}
	}

	return false;
}


REGISTER_WIDGET(WidgetWaveform,
	.name = "waveform",
	.description = "Waveform display",
	.hotkey = ImGuiKey_F1,
	.flags = Widget::Info::Flags::ShowChannelMap | Widget::Info::Flags::ShowLock,
);

