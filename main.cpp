
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <fftw3.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "panel.hpp"
#include "stream.hpp"
#include "stream-reader-audio.hpp"
#include "stream-reader-file.hpp"
#include "stream-reader-generator.hpp"
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

	void init_video();
	void req_redraw();

	void draw();
	void resize_window(int w, int h);

private:
	Panel *m_root_panel;

	SDL_Window *m_win{};
	SDL_Texture *m_tex{};
	SDL_Renderer *m_rend{};
	bool m_resize{true};
	int m_w = 800;
	int m_h = 600;
	Time m_srate{48000.0};
	Streams m_streams;
	View m_view{};
	int m_redraw{1};

	bool m_capturing{true};
	bool m_playback{false};
};


Corrie::Corrie(SDL_Window *window, SDL_Renderer *renderer)
    : m_win(window)
    , m_rend(renderer)
	, m_streams()
{
    resize_window(800, 600);
	m_view.srate = m_srate;
}


void Corrie::load(const char *fname)
{
	ConfigReader cr;
	cr.open(fname);
	if(auto n = cr.find("config")) n->read("samplerate", m_srate);
	if(auto n = cr.find("view")) m_view.load(n);
	if(auto n = cr.find("panel")) m_root_panel->load(n);
	if(auto n = cr.find("stream")) m_streams.load(n);
}


void Corrie::save(const char *fname)
{
	ConfigWriter cw;
	cw.open(fname);

	cw.push("config");
	cw.write("samplerate", m_srate);
	cw.pop();
	
	cw.push("stream");
	m_streams.save(cw);
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
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	SDL_SetRenderTarget(m_rend, m_tex);
	SDL_SetRenderDrawColor(m_rend, 10, 10, 10, 255);
	SDL_RenderClear(m_rend);

	m_root_panel->draw(m_view, m_streams, m_rend, 0, 0, m_w, m_h);

	SDL_SetRenderTarget(m_rend, nullptr);

	ImGui::Render();
	ImGuiIO& io = ImGui::GetIO();
	SDL_SetRenderScale(m_rend, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
	SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 255);
	SDL_RenderClear(m_rend);
	SDL_RenderTexture(m_rend, m_tex, nullptr, nullptr);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_rend);
	SDL_RenderPresent(m_rend);
}


void Corrie::init()
{
	fcntl(0, F_SETFL, O_NONBLOCK);

	init_video();

	m_view.srate = m_srate;


	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.LogFilename = NULL;

	Panel *tmp_root_panel = new Panel(Panel::Type::Root);
	m_root_panel = tmp_root_panel;

	load("config.txt");

	if(m_root_panel->nkids() == 0) {
		Panel *p2 = new Panel(Panel::Type::SplitH);
		p2->add(Widgets::create_widget("waterfall"));
		p2->add(Widgets::create_widget("spectrum"));
		Panel *p1 = new Panel(Panel::Type::SplitV);
		p1->add(p2);
		p1->add(Widgets::create_widget("waveform"));
		p1->add(Widgets::create_widget("waveform"));
		m_root_panel->add(p1);
	}

	int fd1 = open("/home/ico/tmp/1.s16", O_RDONLY);
	m_streams.add_reader(new StreamReaderFile(2, fd1));
	int fd2 = open("/home/ico/tmp/2.s16", O_RDONLY);
	m_streams.add_reader(new StreamReaderFile(2, fd2));
	//m_streams.add_reader(new StreamReaderAudio(3, m_srate));
	//m_streams.add_reader(new StreamReaderGenerator(1, m_srate, 1));
	//m_streams.add_reader(new StreamReaderGenerator(1, m_srate, 1));
	m_streams.allocate(512 * 1024 * 1024);
	m_streams.capture_enable(true);
	m_streams.player.seek(m_view.time.playpos);
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


void Corrie::req_redraw()
{
	m_redraw = 40; // TODO for smooth key zoom/pan
}


void Corrie::run()
{
	bool done = false;
	while (!done)
	{

		if(ImGui::IsKeyPressed(ImGuiKey_C)) {
			m_capturing ^= 1;
			m_streams.capture_enable(m_capturing);
		}

		if(ImGui::IsKeyPressed(ImGuiKey_Space)) {
			m_playback ^= 1;
			m_streams.player.enable(m_playback);
		}

		auto &player = m_streams.player;
		float factor = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 2.0 : 1.059463;
		float stretch = player.get_stretch();
		float pitch = player.get_pitch();
		if(ImGui::IsKeyPressed(ImGuiKey_Comma)) {
			player.set_stretch(stretch / factor);
			player.set_pitch(pitch / factor);
		}
		if(ImGui::IsKeyPressed(ImGuiKey_Slash)) {
			player.set_stretch(stretch * factor);
			player.set_pitch(pitch * factor);
		}
		if(ImGui::IsKeyPressed(ImGuiKey_Period)) {
			player.set_pitch(1.0);
			player.set_stretch(1.0);
		}


		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
				done = true;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && 
				event.window.windowID == SDL_GetWindowID(m_win))
				done = true;
			if(event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_Q)
				done = true;
			if(event.type == SDL_EVENT_WINDOW_RESIZED && 
			   event.window.windowID == SDL_GetWindowID(m_win))
				resize_window(event.window.data1, event.window.data2);
			if(event.type == SDL_EVENT_USER) {
				if(event.user.code == k_user_event_audio_capture) {
					size_t frame_idx = (size_t)(uintptr_t)event.user.data1;
					Time t_to = frame_idx / m_view.srate;
					Time dt = t_to - m_view.time.to;
					if(0) {
						m_view.time.from += dt;
						m_view.time.to += dt;
					}
				}
				if(event.user.code == k_user_event_audio_playback) {
					size_t frame_idx = (size_t)(uintptr_t)event.user.data1;
					m_view.time.playpos = frame_idx / m_srate;
					size_t frame_count = (size_t)(uintptr_t)event.user.data2;
					Time dt = (frame_count / m_srate);
					m_view.time.from += dt;
					m_view.time.to += dt;
				}
			}

			req_redraw();
		}

		if(m_redraw > 0) {
			draw();
			m_redraw --;
		} else {
			SDL_Delay(10);
		}
	}
}


void Corrie::exit()
{
	save("config.txt");

	delete m_root_panel;

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(m_rend);
	SDL_DestroyWindow(m_win);
}


int main(int, char**)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	// scope to ensure Corrie destructor called before SDL_Quit
	{
	Corrie cor = Corrie(nullptr, nullptr);
	cor.init();
	cor.run();
	cor.exit();
	}

	fftwf_cleanup();
	SDL_Quit();

	return 0;
}
