
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetinfo.hpp"
#include "widget-channels.hpp"


WidgetChannels::WidgetChannels(WidgetInfo &info)
	: Widget(info)
{
}


WidgetChannels::~WidgetChannels()
{
}



void WidgetChannels::do_draw(Streams &streams, SDL_Renderer *rend, SDL_Rect &r)
{
	auto &channels = streams.player.get_channels();
	auto &player = streams.player;
	float speed = player.get_speed();

	if(channels.size() != streams.channel_count()) {
		return;
	}

	ImGui::NewLine();
	ImGui::NewLine();

	for(size_t ch=0; ch<streams.channel_count(); ch++) {
		channels[ch].enabled = false;
	}

	ImGui::Text("speed:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150);
	if(ImGui::SliderFloat("##speed", &speed, 0.25, 4.0, "Speed %.2fx", ImGuiSliderFlags_Logarithmic)) {
		player.set_speed(speed);
	}
	ImGui::SameLine();
	float semitones = 12.0f * log2f(speed);
	ImGui::Text("%+.2f semitones", semitones);

	ImGui::NewLine();

	for(size_t ch=0; ch<streams.channel_count(); ch++) {
		SDL_Color col = m_channel_map.ch_color(ch);
		ImVec4 imcol = {col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f};

		channels[ch].enabled = m_channel_map.is_channel_enabled(ch);

		ImGui::TextColored(imcol, "ch %zu", ch);

		ImGui::PushID(ch);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		float db = 20.0f * log10f(std::max(channels[ch].volume, 0.0001f));
		ImGui::SliderFloat("##Volume", &db, -90.0, 0.0, "%.0fdB");
		channels[ch].volume = powf(10.0f, db / 20.0f);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		ImGui::SameLine();
		ImGui::SliderFloat("##Pan", &channels[ch].pan, -1.0f, 1.0f, "Pan %.2f");
		ImGui::PopID();
	}
}


REGISTER_WIDGET(WidgetChannels,
	.name = "channels",
	.description = "Channel settings",
	.hotkey = ImGuiKey_F5,
	.flags = WidgetInfo::Flags::ShowChannelMap,
);

