
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <array>
#include <string.h>
#include <memory>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <vector>
#include <functional>
#include <memory>

#include <fftw3.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "window.hpp"
#include "panel.hpp"
#include "widget.hpp"
#include "stream.hpp"


class Corrie {
public:
    Corrie(SDL_Window *window, SDL_Renderer *renderer);

    Uint32 audio_init();
    void handle_audio(int channels, void *data, int len);
    void draw();
    void resize_window(int w, int h);

	Panel *m_root_panel;

    SDL_Window *m_win;
    SDL_Texture *m_tex;
    SDL_Renderer *m_rend;
	bool m_resize;
	int m_w, m_h;

    int m_srate;

	Streams m_streams;
};


Corrie::Corrie(SDL_Window *window, SDL_Renderer *renderer)
    : m_win(window)
    , m_rend(renderer)
	, m_resize(true)
	, m_w(800)
	, m_h(600)
    , m_srate(48000)
{
    resize_window(800, 600);
}


void Corrie::resize_window(int w, int h)
{
    if(m_tex) SDL_DestroyTexture(m_tex);
	m_w = w;
	m_h = h;
	m_resize = true;
    m_tex = SDL_CreateTexture(SDL_GetRenderer(m_win), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetTextureBlendMode(m_tex, SDL_BLENDMODE_BLEND);
}




void Corrie::draw()
{
    SDL_SetRenderTarget(m_rend, m_tex);
	SDL_SetRenderDrawColor(m_rend, 10, 10, 10, 255);
	SDL_RenderClear(m_rend);
	if(m_root_panel) {
		m_root_panel->draw(m_streams, m_rend, 0, 0, m_w, m_h);
	}
	SDL_SetRenderTarget(m_rend, nullptr);
}



void on_audio_capture_callback(void *userdata, SDL_AudioStream *stream, int len1, int len)
{
	void *data = malloc(len);
	SDL_GetAudioStreamData(stream, data, len);

	SDL_Event ev;
	ev.type = (long)userdata;
	ev.user.code = len;
	ev.user.data1 = data;

	SDL_PushEvent(&ev);
}


Uint32 Corrie::audio_init(void)
{
    SDL_AudioSpec want;

    long evnr = SDL_RegisterEvents(1);

    SDL_memset(&want, 0, sizeof(want));
    want.freq = m_srate;
    want.format = SDL_AUDIO_F32;
    want.channels = 2;

    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &want,
            on_audio_capture_callback,
            (void *)evnr);

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));

    return evnr;
}


void Corrie::handle_audio(int channels, void *data, int bytes)
{
	float *p = (float *)data;
	size_t frames = bytes / (channels * sizeof(float));
	for(size_t i=0; i<frames; i++) {
		for(int c=0; c<channels; c++) {
			m_streams.push(c, *p++);
		}
	}
    free(data);
}



int main(int, char**)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if(window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if(renderer == nullptr)
    {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    Corrie *cor = new Corrie(window, renderer);
    Uint32 ev_audio = cor->audio_init();

	cor->m_root_panel = new Panel(Panel::Type::SplitH);
	cor->m_root_panel->add(new Widget(Widget::Type::Waveform));
	Panel *p2 = new Panel(Panel::Type::SplitV);
	cor->m_root_panel->add(p2);
	p2->add(new Widget(Widget::Type::Waterfall));
	p2->add(new Widget(Widget::Type::Spectrum));
	p2->add(new Widget(Widget::Type::Waveform));


    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if(event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_Q)
                done = true;
            if(event.type == ev_audio)
                cor->handle_audio(2, event.user.data1, event.user.code);
            if(event.type == SDL_EVENT_WINDOW_RESIZED && event.window.windowID == SDL_GetWindowID(window))
                cor->resize_window(event.window.data1, event.window.data2);
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

		cor->draw();
		//ImGui::ShowDemoWindow(nullptr);

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, cor->m_tex, nullptr, nullptr);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

