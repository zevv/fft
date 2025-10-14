
#include <assert.h>
#include <stdlib.h>
#include <math.h>
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





class FFT {
public:
    FFT(int size);
    void run(double *in);
    void draw(SDL_Renderer *rend, SDL_Rect &r);

    int m_size;
	Window m_window;
    double *m_in[2];
	std::vector<double> m_cur[2];
	std::vector<double> m_peak[2];
    fftw_plan m_plan[2];
    fftw_complex *m_out[2];
};


FFT::FFT(int size)
    : m_size(size)
	, m_in{}
	, m_out{}
{
	m_window.configure(Window::Type::Gauss, size, 0.4);
    for(int i=0; i<2; i++) {
		m_cur[i].resize(size/2+1);
		m_peak[i].resize(size/2+1);

		if(m_in[i]) fftw_free(m_in[i]);
		if(m_out[i]) fftw_free(m_out[i]);
        m_in[i] = (double *)fftw_malloc(sizeof(double) * size * 2);
        m_out[i] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (size/2+1));
        m_plan[i] = fftw_plan_dft_r2c_1d(m_size, m_in[i], m_out[i], FFTW_ESTIMATE);
    }
}


void FFT::run(double *in)
{
    for(int i=0; i<m_size; i++) {
		double w = m_window.get_data(i);
        m_in[0][i] = in[i*2+0] * w;
        m_in[1][i] = in[i*2+1] * w;
    }
	for(int j=0; j<2; j++) {
		fftw_execute(m_plan[j]);
		for(int i=0; i<m_size/2+1; i++) {
			double real = m_out[0][i][0];
			double imag = m_out[0][i][1];
			m_cur[j][i] = 20 * log10(hypot(real, imag)) + 50;
			m_peak[j][i] = m_cur[j][i] > m_peak[j][i] ? m_cur[j][i] : m_peak[j][i] * 0.99;
		}
	}
}


void FFT::draw(SDL_Renderer *rend, SDL_Rect &r)
{
    SDL_FRect fr = {
        (float)r.x,
        (float)r.y,
        (float)r.w,
        (float)r.h
    };

	SDL_FPoint p_cur[m_size];
    SDL_FPoint p_peak[m_size];

    SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
    SDL_RenderFillRect(rend, &fr);

	for(int c=0; c<2; c++) {
		for(int i=1; i<m_size/2; i++) {
			float x = r.x + (float)i * (r.w / (m_size / 2.0f));
			p_cur[i].x = x;
			p_cur[i].y = r.y + r.h - (m_cur[c][i] / 60.0f) * r.h;
			p_peak[i].x = x;
			p_peak[i].y = r.y + r.h - (m_peak[c][i] / 60.0f) * r.h;
		}

		if(c == 0) {
			SDL_SetRenderDrawColor(rend, 76/2, 229/2, 0, 255);
		} else {
			SDL_SetRenderDrawColor(rend, 178/2, 0, 229/2, 255);
		}
		SDL_RenderLines(rend, p_cur, m_size / 2);
		if(c == 0) {
			SDL_SetRenderDrawColor(rend, 76, 229, 0, 255);
		} else {
			SDL_SetRenderDrawColor(rend, 178, 0, 229, 255);
		}
		SDL_RenderLines(rend, p_peak, m_size / 2);
	}
}


class Corrie {
public:
    Corrie(SDL_Window *window, SDL_Renderer *renderer);

    Uint32 audio_init();
    void handle_audio(void *data, int len);
    void draw();
    void set_size(int w, int h);

	Panel *m_root_panel;

    SDL_Window *m_win;
    SDL_Texture *m_tex;
    SDL_Renderer *m_rend;
	bool m_resize;
	int m_w, m_h;

    int m_srate;
	FFT m_fft;

    size_t m_buf_size;
    size_t m_buf_pos;
    double m_buf[2][1024];
};


Corrie::Corrie(SDL_Window *window, SDL_Renderer *renderer)
    : m_win(window)
    , m_rend(renderer)
	, m_resize(true)
	, m_w(800)
	, m_h(600)
    , m_srate(48000)
	, m_fft(1024)
    , m_buf_size(1024)
    , m_buf_pos(0)
{
    set_size(800, 600);
}


void Corrie::set_size(int w, int h)
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
	SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 0);
	SDL_RenderClear(m_rend);

	if(m_root_panel) {
		m_root_panel->draw(m_rend, 0, 0, m_w, m_h);
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


void Corrie::handle_audio(void *data, int bytes)
{
    int frames = bytes / (sizeof(float) * 2);
    float *fdata = (float *)data;
    for(int i=0; i<frames; i++) {
        m_buf[0][m_buf_pos] = fdata[i*2+0];
        m_buf[1][m_buf_pos] = fdata[i*2+1];
        m_buf_pos++;
        if(m_buf_pos >= m_buf_size) {
            m_buf_pos = 0;
			m_fft.run(m_buf[0]);
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
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
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
	p2->add(new Widget(Widget::Type::Spectogram));


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
                cor->handle_audio(event.user.data1, event.user.code);
            if(event.type == SDL_EVENT_WINDOW_RESIZED && event.window.windowID == SDL_GetWindowID(window))
                cor->set_size(event.window.data1, event.window.data2);
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
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderTexture(renderer, cor->m_tex, nullptr, nullptr);
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

