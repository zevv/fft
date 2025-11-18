
#include <math.h>
#include <algorithm>

#include "view.hpp"


void View::load(ConfigReader::Node *n)
{
	n->read("lock", lock);
	if(auto *nc = n->find("time")) {
		nc->read("cursor", time.cursor);
		nc->read("playpos", time.playpos);
		nc->read("from", time.from);
		nc->read("to", time.to);
		nc->read("sel_from", time.sel_from);
		nc->read("sel_to", time.sel_to);
	}
	if(auto *nc = n->find("frequency")) {
		nc->read("cursor", freq.cursor);
		nc->read("from", freq.from);
		nc->read("to", freq.to);
	}
	if(auto *nc = n->find("amplitude")) {
		nc->read("cursor", amplitude.cursor);
		nc->read("from", amplitude.from);
		nc->read("to", amplitude.to);
	}
	if(auto *nc = n->find("window")) {
		nc->read("size", window.size);
		if(const char *tmp = nc->read_str("type")) {
			window.window_type = Window::str_to_type(tmp);
		}
		nc->read("beta", window.window_beta);
	}
}

void View::save(ConfigWriter &cfg)
{
	cfg.write("lock", lock);
	cfg.push("time");
	cfg.write("cursor", time.cursor);
	cfg.write("playpos", time.playpos);
	cfg.write("from", time.from);
	cfg.write("to", time.to);
	cfg.write("sel_from", time.sel_from);
	cfg.write("sel_to", time.sel_to);
	cfg.pop();
	cfg.push("frequency");
	cfg.write("from", freq.from);
	cfg.write("to", freq.to);
	cfg.write("cursor", freq.cursor);
	cfg.pop();
	cfg.push("amplitude");
	cfg.write("from", amplitude.from);
	cfg.write("to", amplitude.to);
	cfg.write("cursor", amplitude.cursor);
	cfg.pop();
	cfg.push("window");
	cfg.write("size", window.size);
	cfg.write("type", Window::type_to_str(window.window_type));
	cfg.write("beta", window.window_beta);
	cfg.pop();
}

void View::set_cursor(Config &cfg, SDL_Rect &r, ImVec2 pos)
{
	if(cfg.x == Axis::Time) time.cursor = time.from + (time.to - time.from) * (pos.x - r.x) / r.w;
	if(cfg.y == Axis::Time) time.cursor = time.from + (time.to - time.from) * (pos.y - r.y) / r.h;
	if(cfg.x == Axis::Frequency) freq.cursor = freq.from + (freq.to - freq.from) * (pos.x - r.x) / r.w;
	if(cfg.y == Axis::Frequency) freq.cursor = freq.from + (freq.to - freq.from) * (1.0 - (pos.y - r.y) / r.h);
	if(cfg.x == Axis::Amplitude) amplitude.cursor = amplitude.from + (amplitude.to - amplitude.from) * (pos.x - r.x) / r.w;
	if(cfg.y == Axis::Amplitude) amplitude.cursor = amplitude.from + (amplitude.to - amplitude.from) * (1.0 - (pos.y - r.y) / r.h);
}

void View::move_cursor(Config &cfg, SDL_Rect &r, ImVec2 delta)
{
	if(cfg.x == Axis::Time) time.cursor += delta.x / r.w * (time.to - time.from) * 0.1;
	if(cfg.y == Axis::Time) time.cursor += delta.y / r.h * (time.to - time.from) * 0.1;
	if(cfg.x == Axis::Frequency) freq.cursor += delta.x / r.w * (freq.to - freq.from) * 0.1;
	if(cfg.y == Axis::Frequency) freq.cursor -= delta.y / r.h * (freq.to - freq.from) * 0.1;
}

float View::from_t(Config &cfg, SDL_Rect &r, Time t)
{
	if(cfg.x == Axis::Time) return r.x + r.w * (t - time.from) / (time.to - time.from);
	if(cfg.y == Axis::Time) return r.y + r.h * (t - time.from) / (time.to - time.from);
	return 0.0;
}

float View::from_freq(Config &cfg, SDL_Rect &r, Frequency f)
{
	if(cfg.x == Axis::Frequency) return r.x + r.w * (f- freq.from) / (freq.to - freq.from);
	if(cfg.y == Axis::Frequency) return r.y + r.h * (1.0 - (f - freq.from) / (freq.to - freq.from));
	return 0.0;
}

void View::pan(Config &cfg, SDL_Rect &r, ImVec2 delta)
{
	double dx = delta.x / r.w;
	double dy = delta.y / r.h;

	double ft = 0.0;
	if(cfg.x == Axis::Time) ft = dx;
	if(cfg.y == Axis::Time) ft = dy;
	time.from += -ft * (time.to - time.from);
	time.to   += -ft * (time.to - time.from);

	double ff = 0.0;
	if(cfg.x == Axis::Frequency) ff = -dx;
	if(cfg.y == Axis::Frequency) ff =  dy;
	freq.from += ff * (freq.to - freq.from);
	freq.to   += ff * (freq.to - freq.from);

	double fa = 0.0;
	if(cfg.x == Axis::Amplitude) fa = -dx;
	if(cfg.y == Axis::Amplitude) fa = dy;
	amplitude.from += fa * (amplitude.to - amplitude.from);
	amplitude.to   += fa * (amplitude.to - amplitude.from);

	clamp();
}


void View::zoom_start()
{
	time.zoom_start = time.cursor;
	freq.zoom_start = freq.cursor;
	amplitude.zoom_start = amplitude.cursor;
}


void View::zoom(Config &cfg, SDL_Rect &r, ImVec2 delta)
{
	double ft = 0.0;
	if(cfg.x == Axis::Time) ft = delta.x;
	if(cfg.y == Axis::Time) ft = delta.y;
	time.from += (time.zoom_start - time.from) * ft * 0.01;
	time.to   -= (time.to - time.zoom_start) * ft * 0.01;

	double ff = 0.0;
	if(cfg.x == Axis::Frequency) ff = delta.x;
	if(cfg.y == Axis::Frequency) ff = delta.y;
	freq.from += (freq.zoom_start - freq.from) * ff * 0.01;
	freq.to   -= (freq.to - freq.zoom_start) * ff * 0.01;

	double fa = 0.0;
	if(cfg.x == Axis::Amplitude) fa = delta.x;
	if(cfg.y == Axis::Amplitude) fa = delta.y;
	amplitude.from += (amplitude.zoom_start - amplitude.from) * fa * 0.01;
	amplitude.to   -= (amplitude.to - amplitude.zoom_start) * fa * 0.01;

	clamp();
}

void View::pan_t(float f)
{
	Time dt = -f * (time.to - time.from);
	time.from += dt;
	time.to += dt;
	clamp();
}

void View::zoom_t(float f)
{
	Time t_mid = (time.from + time.to) * 0.5;
	time.from += (t_mid - time.from) * f * 0.01;
	time.to   -= (time.to - t_mid) * f * 0.01;
	clamp();
}

void View::pan_freq(float f)
{
	freq.from += f * (freq.to - freq.from);
	freq.to   += f * (freq.to - freq.from);
	clamp();
}

void View::clamp()
{
	if(time.from >= time.to) {
		Time mid = 0.5 * (time.from + time.to);
		time.from = mid - 0.001;
		time.to   = mid + 0.001;
	}
	if(freq.from >= freq.to) {
		Frequency mid = 0.5 * (freq.from + freq.to);
		freq.from = mid - 0.001;
		freq.to   = mid + 0.001;
	}

	window.size = std::clamp(powf(2.0, floor(log2(window.size) + 0.5)), 8.0f, 32768.0f);
}

