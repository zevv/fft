
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
	float stretch = player.get_stretch();
	float pitch = player.get_pitch();

	if(channels.size() != streams.channel_count()) {
		return;
	}

	if(ImGui::BeginTabBar("Channels")) {

		if(ImGui::BeginTabItem("Playback")) {

			ImGui::NewLine();

			ImGui::SetNextItemWidth(150);
			if(ImGui::SliderFloat("##stretch", &stretch, 0.25, 4.0, "Stretch %.2fx", ImGuiSliderFlags_Logarithmic)) {
				player.set_stretch(stretch);
			}
			ImGui::SameLine();
			if(ImGui::Button("0##stretch")) {
				player.set_stretch(1.0f);
			}

			ImGui::SetNextItemWidth(150);
			if(ImGui::SliderFloat("##pitch", &pitch, 0.25, 4.0, "Pitch %.2fx", ImGuiSliderFlags_Logarithmic)) {
				player.set_pitch(pitch);
			}
			ImGui::SameLine();
			if(ImGui::Button("-##pitch")) {
				player.set_pitch(pitch / powf(2.0f, 1.0f / 12.0f));
			}
			ImGui::SameLine();
			if(ImGui::Button("0##pitch")) {
				player.set_pitch(1.0f);
			}
			ImGui::SameLine();
			if(ImGui::Button("+##pitch")) {
				player.set_pitch(pitch * powf(2.0f, 1.0f / 12.0f));
			}
			ImGui::SameLine();
			float semitones = 12.0f * log2f(pitch);
			ImGui::Text("%+.2f semitones", semitones);

			ImGui::NewLine();

			for(size_t ch=0; ch<streams.channel_count(); ch++) {
				SDL_Color col = m_channel_map.ch_color(ch);
				ImVec4 imcol = {col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f};

				ImGui::TextColored(imcol, "ch %zu", ch+1);

				ImGui::PushID(ch);

				ImGui::SameLine();
				ImGui::Checkbox("##Enable", &channels[ch].enabled);

				ImGui::SameLine();
				ImGui::SetNextItemWidth(150);
				float db = 20.0f * log10f(std::max(channels[ch].gain, 0.0001f));
				ImGui::SliderFloat("##Volume", &db, -60.0, 0.0, "%.0fdB");
				channels[ch].gain = db > -60 ? powf(10.0f, db / 20.0f) : 0.0f;

				ImGui::SameLine();
				ImGui::SetNextItemWidth(150);
				ImGui::SameLine();
				ImGui::SliderFloat("##Pan", &channels[ch].pan, -1.0f, 1.0f, "Pan %.2f");
				ImGui::PopID();
			}
			ImGui::EndTabItem();
		}

		if(ImGui::BeginTabItem("colors")) {

			ImGui::NewLine();

			for(size_t ch=0; ch<streams.channel_count(); ch++) {
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

			for(size_t x=0; x<streams.channel_count(); x++) {
				for(size_t y=0; y<streams.channel_count(); y++) {
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

			ImGui::EndTabItem();
		}


		ImGui::EndTabBar();
	}
}


REGISTER_WIDGET(WidgetChannels,
	.name = "channels",
	.description = "Channel settings",
	.hotkey = ImGuiKey_F5,
);
