
#pragma once 

#include "panel.hpp"
#include "stream.hpp"
#include "misc.hpp"
#include "view.hpp"
#include "config.hpp"
#include "widgetregistry.hpp"
#include "app.hpp"


class App {
public:
	App(SDL_Window *window, SDL_Renderer *renderer);

	void config_fname(char *buf, size_t buflen);
	void load();
	void save();

	void play_toggle();
	void capture_toggle();

	void init(int argc, char **argv);
	void usage();
	void run();
	void exit();
	void handle_keys();

	void init_video();
	void req_redraw();

	void draw();
	int draw_topbar();
	void draw_help_overlay();
	void draw_bottombar();
	void resize_window(int w, int h);

private:
	Panel *m_root_panel;

	SDL_Window *m_win{};
	SDL_Renderer *m_rend{};
	bool m_resize{true};
	int m_w = 800;
	int m_h = 600;
	Time m_srate{48000.0};
	Stream m_stream;
	View m_view{};
	int m_redraw{1};
	char m_session_name[64]{"default"};

	bool m_capturing{false};
	bool m_playback{false};
	bool m_transport{true};
};


