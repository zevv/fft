
#include <string>
#include <math.h>
#include <assert.h>
#include <vector>
#include <algorithm>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "widgetregistry.hpp"
#include "misc.hpp"


class WidgetSources : public Widget {

public:
	WidgetSources(Widget::Info &info);
	~WidgetSources() override;

private:

	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;

	std::vector<Sample> m_vu_peak{};
};


WidgetSources::WidgetSources(Widget::Info &info)
	: Widget(info)
{
}


WidgetSources::~WidgetSources()
{
}




void WidgetSources::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	size_t ch = 0;

	auto &sources = stream.capture.sources();

	ImGui::NewLine();

	for(auto &source : sources) {
			
		char specstr[32];
		SDL_AudioSpec aspec = source->src_spec();
		sdl_audiospec_to_str(aspec, specstr, sizeof(specstr));
		char label[64];
		snprintf(label, sizeof(label), "%s / %s", source->info().description, specstr);

		if(ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
				
			ImGui::SetNextItemWidth(150);
			float gain_db = gain_to_db(source->gain());
			ImGui::PushID(ch);
			ImGui::SliderFloat("##gain", &gain_db, -60.0f, 30.0f, "%+.1f dB");
			source->set_gain(db_to_gain(gain_db));
			ImGui::PopID();

			for(size_t i=0; i<source->channel_count(); i++) {
				ImGui::PushID(ch);
				char chan_label[32];
				snprintf(chan_label, sizeof(chan_label), "%zu", ch+i);

				SDL_Color col = m_channel_map.ch_color(ch);
				ImVec4 imcol = {col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f};
				ImGui::TextColored(imcol, "%zu", ch);

				ImGui::PopID();
				ch ++;
			}

		}
	}
}



REGISTER_WIDGET(WidgetSources,
	.name = "sources",
	.description = "source controls",
	.hotkey = ImGuiKey_F10,
);
