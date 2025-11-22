
// TODO:
// - consolidate color types

#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <fftw3.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "app.hpp"
#include "style.hpp"


App::App(SDL_Window *window, SDL_Renderer *renderer)
    : m_win(window)
    , m_rend(renderer)
	, m_stream()
{
    resize_window(800, 600);
}



void App::config_fname(char *buf, size_t buflen)
{
	char dir[PATH_MAX];
	const char *path = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");
	if(path) {
		snprintf(dir, sizeof(dir), "%s/fft", path);
	} else if(home) {
		snprintf(dir, sizeof(dir), "%s/.config/fft", home);
	} else {
		snprintf(dir, sizeof(dir), "./.fft");
	}
	mkdir(dir, 0755);
	snprintf(buf, buflen, "%s/%s", dir, m_session_name);
}


void App::load()
{
	char fname[PATH_MAX];
	config_fname(fname, sizeof(fname));

	ConfigReader cr;
	cr.open(fname);
	if(auto n = cr.find("config")) {
		n->read("samplerate", m_srate);
		n->read("session", m_session_name, sizeof(m_session_name));
	}
	if(auto n = cr.find("view")) m_view.load(n);
	if(auto n = cr.find("panel")) m_root_panel->load(n);
	if(auto n = cr.find("stream")) m_stream.load(n);
	if(auto n = cr.find("style")) Style::load(n);
}


void App::save()
{
	char fname[PATH_MAX];
	config_fname(fname, sizeof(fname));

	ConfigWriter cw;
	cw.open(fname);

	cw.push("config");
	cw.write("samplerate", m_srate);
	cw.write("session", m_session_name);
	cw.pop();
	
	cw.push("stream");
	m_stream.save(cw);
	cw.pop();

	cw.push("view");
	m_view.save(cw);
	cw.pop();

	cw.push("style");
	Style::save(cw);
	cw.pop();

	cw.push("panel");
	m_root_panel->save(cw);
	cw.pop();

	cw.close();
}


void App::resize_window(int w, int h)
{
	m_w = w;
	m_h = h;
	m_resize = true;
}

int App::draw_topbar()
{
	ImGuiWindowFlags flags = 0;
	flags |= ImGuiWindowFlags_NoDecoration;
	ImGui::Begin("main", nullptr, flags);

	bool cap = m_capturing;
	ImGui::ToggleButton("C##capture", &cap);
	if(cap != m_capturing) capture_toggle();

	ImGui::SameLine();

	bool play = m_playback;
	ImGui::ToggleButton("P##playback", &play);
	if(play != m_playback) play_toggle();

	ImGui::SameLine();
	ImGui::Text("srate: %.0fHz", m_srate);

	size_t frames_avail{};
	m_stream.peek(nullptr, &frames_avail);
	ImGui::SameLine();
	char buf[32];
	duration_to_str(frames_avail / m_srate, buf, sizeof(buf));
	float bytes = m_stream.channel_count() * sizeof(Sample) * frames_avail;
	ImGui::Text("| capture: %s, %.1fMb", buf, bytes / (1024.0 * 1024.0));

	auto io = ImGui::GetIO();
	ImGui::SameLine();
	ImGui::Text("| %.1f fps", io.Framerate);

	int y = ImGui::GetWindowHeight();
	ImGui::End();

	return y;
}


void App::draw_bottombar()
{
	ImGuiWindowFlags flags = 0;
	flags |= ImGuiWindowFlags_NoDecoration;
	ImGui::Begin("bottombar", nullptr, flags);

	if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
		ImGui::Text("LMB:           MMB:            RMB: zoom");
	} else {
		ImGui::Text("LMB: play pos  MMB:            RMB: pan");
	}

	ImGui::End();
}



void App::draw()
{
	SDL_SetRenderDrawColor(m_rend, Style::color(Style::ColorId::Background));
	SDL_RenderClear(m_rend);

	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8, 8});
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

	ImGui::SetNextWindowPos(ImVec2{0, 0});
	ImGui::SetNextWindowSizeConstraints(ImVec2(m_w, -1), ImVec2(m_w, -1));
	float bar_h = draw_topbar();

	m_root_panel->draw(m_view, m_stream, m_rend, 0, bar_h + 1, m_w, m_h - bar_h - bar_h + 1);

	ImGui::SetNextWindowPos(ImVec2{0, m_h - bar_h + 2});
	ImGui::SetNextWindowSize(ImVec2(m_w, bar_h - 4));
	draw_bottombar();

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();

	ImGui::Render();
	ImGuiIO& io = ImGui::GetIO();
	SDL_SetRenderScale(m_rend, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_rend);
	SDL_RenderPresent(m_rend);
}


void App::usage(void)
{
	fprintf(stderr, 
		"usage: fft [options] <src> ...\n"
		"\n"
		"options:\n"
		"  -d --buffer-depth N  set buffer depth to N bytes (default: 512MB)\n"
		"  -h                   show help\n"
		"  -r --sample-rate N   set sample rate to N (default: 48000)\n"
		"\n"
		"sources:\n"
		"  raw:FILENAME[:AUDIOSPEC]\n"
		"  stdin[:AUDIOSPEC]\n"
		"  audio[:CHANNELS]\n"
		"\n"
		"audio spec:\n"
		"  u8|s16|s32|f32|f64[:CHANNELS][:SRATE]\n"

	);
}

void App::init(int argc, char **argv)
{
	fcntl(0, F_SETFL, O_NONBLOCK);

	init_video();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.LogFilename = NULL;

	Panel *tmp_root_panel = new Panel(Panel::Type::Root);
	m_root_panel = tmp_root_panel;

	load();

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

	static struct option long_options[] = {
		{"help",          no_argument,       0, 'h'},
		{"sample-rate",   required_argument, 0, 'r'},
		{"buffer-depth",  required_argument, 0, 'd'},
		{"session",       required_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	int opt_buffer_depth = 512 * 1024 * 1024;

	int opt;
	while ((opt = getopt_long(argc, argv, "d:hr:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'd':
				opt_buffer_depth = atof(optarg);
				break;
			case 'h':
				usage();
				::exit(0);
				break;
			case 'r':
				m_srate = atof(optarg);
				break;
			case 's':
				snprintf(m_session_name, sizeof(m_session_name), "%s", optarg);
				break;
			case 0:
				printf("option %s", long_options[optind].name);
				break;
			default:
				usage();
				::exit(1);
		}
	}
	
	m_stream.set_sample_rate(m_srate);
	
	for(int i=optind; i<argc; i++) {
		m_stream.capture.add_source(argv[i]);
	}

	if(m_stream.capture.channel_count() == 0) {
		fprintf(stderr, "error: no input sources specified\n");
		::exit(1);
	}
	
	m_stream.allocate(opt_buffer_depth);
	m_stream.capture.start();
	m_stream.capture.resume();
	m_stream.player.seek(m_view.time.playpos);
}


void App::init_video(void)
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


void App::req_redraw()
{
	m_redraw = 40; // TODO for smooth key zoom/pan
}


void App::play_toggle()
{
	m_playback ^= 1;
	if(m_playback) {
		m_stream.player.resume();
	} else {
		m_stream.player.pause();
	}
}


void App::capture_toggle()
{
	m_capturing ^= 1;
	if(m_capturing) {
		m_stream.capture.resume();
	} else {
		m_stream.capture.pause();
	}
}


void App::run()
{
	bool done = false;
	while (!done)
	{

		// Capture control
		if(ImGui::IsKeyPressed(ImGuiKey_C)) {
			capture_toggle();
		}

		// Player control
		auto &player = m_stream.player;

		if(ImGui::IsKeyPressed(ImGuiKey_Space) || ImGui::IsKeyPressed(ImGuiKey_P)) {
			play_toggle();
		}
			
		auto cfg = player.config();

		float factor = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 2.0 : 1.059463;
		
		if(ImGui::IsKeyPressed(ImGuiKey_Comma)) {
			cfg.pitch /= factor;
			cfg.stretch /= factor;
		}
		if(ImGui::IsKeyPressed(ImGuiKey_Slash)) {
			cfg.pitch *= factor;
			cfg.stretch *= factor;
		}
		if(ImGui::IsKeyPressed(ImGuiKey_Period)) {
			cfg.shift = 0.0;
			cfg.pitch = 1.0;
			cfg.stretch = 1.0;
		}
		player.set_config(cfg);


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
					Time t_to = frame_idx / m_srate;
					Time dt = t_to - m_view.time.to;
					if(!m_playback) {
						m_view.time.from += dt;
						m_view.time.to += dt;
						m_view.time.analysis = m_view.time.to - m_view.window.size / m_srate * 0.5;
					}
					size_t used = 0;
					m_stream.peek(nullptr, &used);
				}
				if(event.user.code == k_user_event_audio_playback) {
					ssize_t frame_idx = (size_t)(uintptr_t)event.user.data1;
					m_view.time.playpos = frame_idx / m_srate;
					size_t frame_count = (size_t)(uintptr_t)event.user.data2;
					Time dt = (frame_count / m_srate);
					m_view.time.from += dt;
					m_view.time.to += dt;
					m_view.time.analysis = m_view.time.playpos;
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


void App::exit()
{
	save();

	delete m_root_panel;

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(m_rend);
	SDL_DestroyWindow(m_win);
}
