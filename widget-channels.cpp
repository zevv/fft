
#include <string>
#include <math.h>
#include <assert.h>
#include <vector>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetregistry.hpp"


class WidgetChannels : public Widget {

public:
	WidgetChannels(Widget::Info &info);
	~WidgetChannels() override;

private:

	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	void do_draw_playback_tab(Stream &stream, SDL_Renderer *rend, SDL_Rect &r);
	void do_draw_colors_tab(Stream &stream, SDL_Renderer *rend, SDL_Rect &r);
	void draw_vu(Stream &stream, size_t channel, ImVec4 col);

	ssize_t m_vu_idx_prev{0};
	std::vector<Sample> m_vu_peak{};
};


WidgetChannels::WidgetChannels(Widget::Info &info)
	: Widget(info)
{
}


WidgetChannels::~WidgetChannels()
{
}



void WidgetChannels::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{

	if(ImGui::BeginTabBar("Channels")) {

		if(ImGui::BeginTabItem("Playback")) {
			do_draw_playback_tab(stream, rend, r);
			ImGui::EndTabItem();
		}

		if(ImGui::BeginTabItem("colors")) {
			do_draw_colors_tab(stream, rend, r);
			ImGui::EndTabItem();
		}
		
		if(ImGui::BeginTabItem("style")) {
			ImGui::ShowStyleEditor();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

}


void WidgetChannels::draw_vu(Stream &stream, size_t ch, ImVec4 col)
{
	ImGui::PushID((int)ch);

	float width = 80;
	float height = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
	ImVec2 pos1, pos2;
	ImGui::SetNextItemWidth(width);
	float val = 0.0;
	ImGui::DragFloat("##vu", &val, 0.0f, 0.0f, 1.0f, "");

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetItemRectMin();
	pos.y += 4;
	pos.x += 4;

	float db_range = 60.0f;
	float db = 20.0f * log10f((float)m_vu_peak[ch]/k_sample_max + 1e-10f);
	float db_clamped = std::clamp(db, -db_range, 0.0f);
	float x = (db_clamped + db_range) / db_range * width;

	for(float i=0; i<x-8; i+=8) {
		draw_list->AddRectFilled(ImVec2(pos.x + i, pos.y), ImVec2(pos.x + i + 4, pos.y + height - 8),
			IM_COL32(64, 64, 64, 255));
	}
	//draw_list->AddRectFilled(pos, ImVec2(pos.x + x - 8, pos.y + height - 8),  IM_COL32(100, 100, 100, 255));
	draw_list->AddRectFilled(ImVec2(pos.x + x - 8, pos.y), ImVec2(pos.x + x - 4, pos.y + height - 8),
		IM_COL32((uint8_t)(col.x * 255.0f), (uint8_t)(col.y * 255.0f), (uint8_t)(col.z * 255.0f), 255));

	ImGui::PopID();
}


void WidgetChannels::do_draw_playback_tab(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	m_vu_peak.resize(stream.channel_count());

	size_t frames_stride;
	size_t frames_avail;
	Sample *frames_data = stream.peek(&frames_stride, &frames_avail);
	ssize_t idx_to   = std::min((ssize_t)(m_view.time.analysis * stream.sample_rate()), (ssize_t)frames_avail);
	ssize_t idx_from = std::max({m_vu_idx_prev, idx_to - 10000, (ssize_t)0 });
	m_vu_idx_prev = idx_to;

	float decay = powf(0.97f, (float)(idx_to - idx_from) / 100.0f);
	
	for(size_t ch=0; ch<stream.channel_count(); ch++) {
		m_vu_peak[ch] *= decay;
		for(ssize_t idx=idx_from; idx<idx_to; idx++) {
			Sample v = frames_data[idx * frames_stride + ch];
			m_vu_peak[ch] = std::max(m_vu_peak[ch], v);
		}
	}





	auto &channels = stream.player.get_channels();
	auto &player = stream.player;
	float stretch = player.get_stretch();
	float pitch = player.get_pitch();

	ImGui::NewLine();

	float gain = player.master_gain_get();
	float db = 20.0f * log10f(std::max(gain, 0.0001f));
	ImGui::SetNextItemWidth(150);
	ImGui::SliderFloat("##Gain", &db, -30.0, +30.0, "master %+.0fdB");
	player.master_gain_set(powf(10.0f, db / 20.0f));

	float fs = stream.sample_rate() * 0.5;
	float f_lp, f_hp;
	player.filter_get(f_lp, f_hp);
	f_lp *= fs;
	f_hp *= fs;
	ImGui::SetNextItemWidth(150);
	ImGui::SliderFloat("##Highpass", &f_hp, 0.0f, fs, "Highpass %.0fHz");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150);
	ImGui::SliderFloat("##Lowpass", &f_lp, 0.0f, fs, "Lowpass %.0fHz");
	player.filter_set(f_lp / fs, f_hp / fs);

	ImGui::SetNextItemWidth(150);
	if(ImGui::SliderFloat("##stretch", &stretch, 0.25, 4.0, "Stretch %.2fx", ImGuiSliderFlags_Logarithmic)) {
		player.set_stretch(stretch);
	}
	ImGui::SameLine();
	if(ImGui::Button("-##stretch")) player.set_stretch(stretch / powf(2.0f, 1.0f / 12.0f));
	ImGui::SameLine();
	if(ImGui::Button("0##stretch")) player.set_stretch(1.0f);
	ImGui::SameLine();
	if(ImGui::Button("+##stretch")) player.set_stretch(stretch * powf(2.0f, 1.0f / 12.0f));

	ImGui::SetNextItemWidth(150);
	if(ImGui::SliderFloat("##pitch", &pitch, 0.25, 4.0, "Pitch %.2fx", ImGuiSliderFlags_Logarithmic)) {
		player.set_pitch(pitch);
	}
	ImGui::SameLine();
	if(ImGui::Button("-##pitch")) player.set_pitch(pitch / powf(2.0f, 1.0f / 12.0f));
	ImGui::SameLine();
	if(ImGui::Button("0##pitch")) player.set_pitch(1.0f);
	ImGui::SameLine();
	if(ImGui::Button("+##pitch")) {
		player.set_pitch(pitch * powf(2.0f, 1.0f / 12.0f));
	}
	ImGui::SameLine();
	float semitones = 12.0f * log2f(pitch);
	ImGui::Text("%+.2f semitones", semitones);

	ImGui::NewLine();
	size_t ch = 0;
	for(auto &source : stream.capture.sources()) {
		auto info = source->info();
		auto args = source->args();
		char label[64];
		snprintf(label, sizeof(label), "%s / %s", info.name, args);
		if(ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
			for(size_t i=0; i<source->channel_count(); i++) {

				SDL_Color col = m_channel_map.ch_color(ch);
				ImVec4 imcol = {col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f};

				ImGui::TextColored(imcol, "%zu", ch+1);

				ImGui::SameLine();
				draw_vu(stream, ch, imcol);

				ImGui::PushID(ch);

				ImGui::SameLine();
				ImGui::Checkbox("##Enable", &channels[ch].enabled);

				ImGui::SameLine();
				ImGui::SetNextItemWidth(150);
				float db = 20.0f * log10f(std::max(channels[ch].gain, 0.0001f));
				ImGui::SliderFloat("##Volume", &db, -60.0, +0.0, "%.0fdB");
				channels[ch].gain = db > -60 ? powf(10.0f, db / 20.0f) : 0.0f;

				ImGui::SameLine();
				ImGui::SetNextItemWidth(150);
				ImGui::SameLine();
				ImGui::SliderFloat("##Pan", &channels[ch].pan, -1.0f, 1.0f, "Pan %.2f");
				ImGui::PopID();

				ch ++;
			}

		}

	}
}


void WidgetChannels::do_draw_colors_tab(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{

	ImGui::NewLine();

	for(size_t ch=0; ch<stream.channel_count(); ch++) {
		SDL_Color col = m_channel_map.ch_color(ch);
		ImVec4 imcol = {col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f};

		ImGui::SameLine();
		ImGui::TextColored(imcol, "ch %zu", ch+1);

		ImGui::SameLine();
		ImGui::PushID(ch);
		if(ImGui::ColorEdit3("##color", (float*)&imcol, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs)) {
			SDL_Color newcol = { (uint8_t)(imcol.x * 255.0f), (uint8_t)(imcol.y * 255.0f), (uint8_t)(imcol.z * 255.0f), 255 };
			m_channel_map.ch_set_color(ch, newcol);
		}
		ImGui::PopID();
	}

	// get imgui cursor pos

	int px = ImGui::GetCursorScreenPos().x;
	int py = ImGui::GetCursorScreenPos().y;

	for(size_t x=0; x<stream.channel_count(); x++) {
		for(size_t y=0; y<stream.channel_count(); y++) {
			SDL_FRect rect;
			SDL_Color colx = m_channel_map.ch_color(x);
			SDL_Color coly = m_channel_map.ch_color(y);

			for(size_t xx=0; xx<64; xx++) {
				for(size_t yy=0; yy<64; yy++) {

					SDL_Color mixcol = {
						(uint8_t)std::clamp((colx.r * (xx / 63.0f) + coly.r * (yy / 63.0f)), 0.0f, 255.0f),
						(uint8_t)std::clamp((colx.g * (xx / 63.0f) + coly.g * (yy / 63.0f)), 0.0f, 255.0f),
						(uint8_t)std::clamp((colx.b * (xx / 63.0f) + coly.b * (yy / 63.0f)), 0.0f, 255.0f),
						255
					};
					SDL_SetRenderDrawColor(rend, mixcol.r, mixcol.g, mixcol.b, mixcol.a);

					SDL_RenderPoint(rend, px + (x * 70) + xx, py + (y * 70) + yy);
				}
			}
		}
	}
}


REGISTER_WIDGET(WidgetChannels,
	.name = "channels",
	.description = "Channel settings",
	.hotkey = ImGuiKey_F5,
);
