
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widget-wave.hpp"


WidgetWaveform::WidgetWaveform()
	: Widget(Widget::Type::Waveform)
{
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


Widget *WidgetWaveform::do_copy()
{
	WidgetWaveform *w = new WidgetWaveform();
	w->m_agc = m_agc;
	w->m_peak = m_peak;
	return w;
}


void WidgetWaveform::do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	ImGui::SameLine();
	ImGui::ToggleButton("AGC", &m_agc);
	
	size_t frames_avail;
	size_t data_stride;
	Sample *data = streams.peek(&data_stride, &frames_avail);

	size_t wframes_avail;
	size_t wdata_stride;
	Wavecache::Range *wdata = streams.peek_wavecache(&wdata_stride, &wframes_avail);

	if(ImGui::IsWindowFocused()) {
	
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		ImGui::Text("t=%.4gs", m_view.time.cursor);

		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_view.pan_t(ImGui::GetIO().MouseDelta.x / r.w);
			m_view.zoom_t(ImGui::GetIO().MouseDelta.y);
		}

		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.time.from = 0;
			m_view.time.to   = frames_avail / m_view.srate;
		}

		auto pos = ImGui::GetIO().MousePos;
		if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			m_view.time.cursor += m_view.dx_to_dt(ImGui::GetIO().MouseDelta.x, r) * 0.1;
		} else {
			m_view.time.cursor = m_view.x_to_t(pos.x, r);
		}

		if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
		   (ImGui::GetIO().MouseDelta.x != 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left))) {
			auto pos = ImGui::GetIO().MousePos;
			streams.player.seek(m_view.x_to_t(pos.x, r));
		}
	}

	Sample scale = k_sample_max;
	if(m_agc && m_peak > 0.0f) {
		scale = m_peak / 0.9;
	}

	float idx_from = m_view.x_to_t(r.x,       r) * m_view.srate;
	float idx_to   = m_view.x_to_t(r.x + r.w, r) * m_view.srate;
	float step = (idx_to - idx_from) / r.w;

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);

	for(auto ch : m_channel_map.enabled_channels()) {

		SDL_Color col = m_channel_map.ch_color(ch);
		SDL_SetRenderDrawColor(rend, col.r, col.g, col.b, 255);

		Sample peak;

		if(step < 256) {
			peak = graph(rend, r,
					data + ch, frames_avail, data_stride,
					idx_from, idx_to,
					(Sample)-scale, (Sample)+scale);
		} else {
			peak = graph(rend, r,
					&wdata[ch].min, &wdata[ch].max, 
					frames_avail / 256, wdata_stride * 2,
					idx_from / 256, idx_to / 256,
					(int8_t)(-scale / 256), (int8_t)(+scale / 256));
			peak *= 256;
		}

		m_peak = std::max(m_peak, peak);
	}

	// time grid
	grid_time(rend, r, m_view.x_to_t(r.x, r), m_view.x_to_t(r.x + r.w, r));

	// cursor
	SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
	int cx = m_view.t_to_x(m_view.time.cursor, r);
	SDL_RenderLine(rend, cx, r.y, cx, r.y + r.h);
	
	// play position
	SDL_SetRenderDrawColor(rend, 0, 128, 128, 255);
	int px = m_view.t_to_x(m_view.time.playpos, r);
	SDL_RenderLine(rend, px, r.y, px, r.y + r.h);

	// zero Y
	SDL_SetRenderDrawColor(rend, 100, 100, 100, 255);
	SDL_RenderLine(rend, r.x, r.y + r.h / 2, r.x + r.w, r.y + r.h / 2);
	
	// window
	Window w = Window(m_view.fft.window_type, m_view.fft.size, m_view.fft.window_beta);
	SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
	double w_idx_from = (m_view.time.from - m_view.time.cursor) * m_view.srate;
	double w_idx_to   = (m_view.time.to   - m_view.time.cursor) * m_view.srate;
	graph(rend, r, w.data().data(), w.size(), 1, w_idx_from, w_idx_to, 0.0, +1.0);
	
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	m_peak *= 0.9f;
}
