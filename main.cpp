
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
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
#include "view.hpp"
#include "config.hpp"


class Corrie {
public:
    Corrie(SDL_Window *window, SDL_Renderer *renderer);
	
	void load(const char *fname);
	void save(const char *fname);

	void init();
	void run();
	void exit();
    
	void init_audio();
    void init_video();

    void poll_audio();
    void draw();
    void resize_window(int w, int h);

private:
	Panel *m_root_panel;

    SDL_Window *m_win;
    SDL_Texture *m_tex;
    SDL_Renderer *m_rend;
	bool m_resize;
	int m_w, m_h;

    int m_srate;
	bool m_capture;
	SDL_AudioStream *m_sdl_audiostream;

	Streams m_streams;
	View m_view;
};


Corrie::Corrie(SDL_Window *window, SDL_Renderer *renderer)
    : m_win(window)
    , m_rend(renderer)
	, m_resize(true)
	, m_w(800)
	, m_h(600)
    , m_srate(48000)
	, m_capture(false)
	, m_view()
{
    resize_window(800, 600);
}


void Corrie::load(const char *fname)
{
	ConfigReader cr;
	cr.open(fname);
	if(auto n = cr.find("config")) {
		n->read("samplerate", m_srate, 8000);
	}

	if(auto n = cr.find("view")) {
		m_view.load(n);
	}

	if(auto n = cr.find("panel")) {
		m_root_panel->load(n);
	}

	cr.close();
}


void Corrie::save(const char *fname)
{
	ConfigWriter cw;
	cw.open(fname);

	cw.push("config");
	cw.write("samplerate", m_srate);
	cw.pop();

	cw.push("view");
	m_view.save(cw);
	cw.pop();

	cw.push("panel");
	m_root_panel->save(cw);
	cw.pop();

	cw.close();
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
	m_view.clamp();
    SDL_SetRenderTarget(m_rend, m_tex);
	SDL_SetRenderDrawColor(m_rend, 10, 10, 10, 255);
	SDL_RenderClear(m_rend);
	if(m_root_panel) {
		m_root_panel->draw(m_view, m_streams, m_rend, 0, 0, m_w, m_h);
	}
	SDL_SetRenderTarget(m_rend, nullptr);
}


void Corrie::init()
{
	fcntl(0, F_SETFL, O_NONBLOCK);

	init_video();
	init_audio();

#if 1
	m_capture = true;
#else
	float phase = 0.0;
	for(int i=0; i<m_srate; i++) {
		float f = i / 2.0;
		float v = sinf(2.0 * M_PI * phase);
		phase += f / m_srate;
		float data[8] = { v, v * i/m_srate, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
		m_streams.write(data, 1);
	}
#endif


	m_root_panel = new Panel(Panel::Type::None);

	load("config.txt");

	if(m_root_panel->get_type() == Panel::Type::None) {
		m_root_panel = new Panel(Panel::Type::SplitV);
		Panel *p2 = new Panel(Panel::Type::SplitH, 500);
		m_root_panel->add(p2);
		p2->add(new Widget(Widget::Type::Waterfall), 400);
		p2->add(new Widget(Widget::Type::Spectrum), 150);
		m_root_panel->add(new Widget(Widget::Type::Waveform), 200);
		m_root_panel->add(new Widget(Widget::Type::Waveform), 200);
	}
}


void Corrie::init_video(void)
{
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if(window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        ::exit(1);
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if(renderer == nullptr)
    {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        ::exit(1);
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

	m_win = window;
	m_rend = renderer;

}


void Corrie::init_audio(void)
{
    SDL_AudioSpec want;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = m_srate;
    want.format = SDL_AUDIO_F32;
    want.channels = 2;

    m_sdl_audiostream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &want,
            nullptr,
            (void *)this);

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_sdl_audiostream));
}


void Corrie::poll_audio()
{
	for(;;) {
		int frames_avail_audio = SDL_GetAudioStreamAvailable(m_sdl_audiostream) / (sizeof(float) * 2);
		int n = 0;
		ioctl(0, FIONREAD, &n);
		int frames_avail_stdin = n / (sizeof(float) * 6);
		int frames_avail = std::min(frames_avail_audio, frames_avail_stdin);
		frames_avail = std::min(frames_avail, 256);
		if(frames_avail == 0) break;

		float buf_audio[256 * 2];
		int r1 = SDL_GetAudioStreamData(m_sdl_audiostream, buf_audio, frames_avail * sizeof(float) * 2);
		assert(r1 == (int)(frames_avail * sizeof(float) * 2));

		float buf_stdin[256 * 6];
		int nbytes = frames_avail * sizeof(float) * 6;
		int r2 = read(0, buf_stdin, nbytes);
		assert(r2 == nbytes);
		
		float buf[256][8];
		for(int i=0; i<frames_avail; i++) {
			buf[i][0] = buf_audio[i * 2 + 0];
			buf[i][1] = buf_audio[i * 2 + 1];
			buf[i][2] = buf_stdin[i * 6 + 0];
			buf[i][3] = buf_stdin[i * 6 + 1];
			buf[i][4] = buf_stdin[i * 6 + 2];
			buf[i][5] = buf_stdin[i * 6 + 3];
			buf[i][6] = buf_stdin[i * 6 + 4];
			buf[i][7] = buf_stdin[i * 6 + 5];
		}

		m_streams.write(buf, frames_avail);
	}
}


void Corrie::run()
{
    bool done = false;
    while (!done)
    {
		if(ImGui::IsKeyPressed(ImGuiKey_Space)) {
			m_capture ^= 1;
		}
		if(m_capture) {
			poll_audio();
		}

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_win))
                done = true;
            if(event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_Q)
                done = true;
            if(event.type == SDL_EVENT_WINDOW_RESIZED && event.window.windowID == SDL_GetWindowID(m_win))
                resize_window(event.window.data1, event.window.data2);
        }

		if(ImGui::IsKeyPressed(ImGuiKey_R)) {
			m_view.reset();
		}

        if (SDL_GetWindowFlags(m_win) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

		draw();
		//ImGui::ShowDemoWindow(nullptr);

        ImGui::Render();
		ImGuiIO& io = ImGui::GetIO();
        SDL_SetRenderScale(m_rend, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 255);
        SDL_RenderClear(m_rend);
        SDL_RenderTexture(m_rend, m_tex, nullptr, nullptr);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_rend);
        SDL_RenderPresent(m_rend);
    }

}


void Corrie::exit()
{
	save("config.txt");

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(m_rend);
    SDL_DestroyWindow(m_win);
    SDL_Quit();
}


int main(int, char**)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    Corrie *cor = new Corrie(nullptr, nullptr);
	cor->init();
	cor->run();
	cor->exit();

	return 0;
}

