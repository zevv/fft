
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>
#include <thread>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "widgetregistry.hpp"
#include "histogram.hpp"
#include "fft.hpp"
#include "queue.hpp"



class WidgetWaterfall2 : public Widget {
public:
	WidgetWaterfall2(Widget::Info &info);
	~WidgetWaterfall2() override;

private:

	struct Worker {
		int id;
		std::thread thread;
		Fft fft;
	};

	struct Channel {
		SDL_Texture *tex{nullptr};
		Fft fft;
	};

	enum class JobCmd {
		Gen, Stop
	};

	struct Job {
		JobCmd cmd;
		Sample *data;
		size_t data_stride;
		int id;
		Channel *channel;
		int w, h;
		uint32_t *pixels;
		size_t pitch;
		uint32_t v;
		ssize_t idx;
		ssize_t didx;
		ssize_t idx_max;
	};

	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;
	void gen_waterfall(Stream &stream, SDL_Renderer *rend, SDL_Rect &r);
	void work(int worker_id);
	void allocate_channels(SDL_Renderer *rend, size_t channels, int w, int h);
	void job_run_gen(Job &job);

	std::vector<Channel> m_channels;

	bool m_fft_approximate{true};

	std::vector<Worker *> m_workers;
	Queue<Job> m_job_queue{32};
	Queue<Job> m_result_queue{32};
	size_t m_jobs_in_flight{0};
};



WidgetWaterfall2::WidgetWaterfall2(Widget::Info &info)
	: Widget(info)
{
	for(size_t i=0; i<8; i++) {
		Worker *w = new Worker();
		w->id = (int)i;
		w->thread = std::thread(&WidgetWaterfall2::work, this, w->id);
		m_workers.push_back(w);
	}
}


WidgetWaterfall2::~WidgetWaterfall2()
{
	for(size_t i=0; i<m_workers.size(); i++) {
		Job job;
		job.cmd = JobCmd::Stop;
		job.id = (int)i;
		m_job_queue.push(job);
	}
	for(auto w : m_workers) {
		w->thread.join();
		delete w;
	}
	for(auto &ch : m_channels) {
		if(ch.tex) SDL_DestroyTexture(ch.tex);
	}
}


void WidgetWaterfall2::work(int tid)
{
	while(true) {
		Job job;
		bitline("+ t%d.wait", hirestime(), tid);
		m_job_queue.pop(job, true);
		bitline("- t%d.wait", hirestime(), tid);
		
		bitline("+ t%d.work.%d", hirestime(), tid, job.cmd);

		if(job.cmd == JobCmd::Stop) {
			break;
		}

		if(job.cmd == JobCmd::Gen) {
			bitline("+ t%d.job.gen.%d", hirestime(), tid, job.id);
			job_run_gen(job);
			bitline("- t%d.job.gen.%d", hirestime(), tid, job.id);
		}

		bitline("- t%d.work.%d", hirestime(), tid, job.cmd);
	}
}


void WidgetWaterfall2::allocate_channels(SDL_Renderer *rend, size_t channels, int w, int h)
{
	m_channels.resize(channels);

	for(auto &ch : m_channels) {
		if(!ch.tex || ch.tex->w != w || ch.tex->h != h) {
			if(ch.tex) {
				SDL_DestroyTexture(ch.tex);
			}
			ch.tex = SDL_CreateTexture(
					rend, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
			SDL_SetTextureBlendMode(ch.tex, SDL_BLENDMODE_ADD);
			SDL_SetTextureScaleMode(ch.tex, SDL_SCALEMODE_LINEAR);
		}

		ch.fft.configure(m_view.window.size, m_view.window.window_type, m_view.window.window_beta);
		ch.fft.set_approximate(m_fft_approximate);
	}
}
		

void WidgetWaterfall2::job_run_gen(Job &job)
{
	ssize_t idx = job.idx;
	uint32_t v = job.v;
	int fft_w = job.channel->fft.out_size();
		
	memset(job.pixels, 0, job.pitch * job.h * 4);

	for(int y=0; y<job.h; y++) {

		idx += job.didx;
		if(idx < 0) continue;
		if(idx >= job.idx_max) break;

		auto fft_out = job.channel->fft.run(&job.data[idx], job.data_stride);
		uint32_t *p = job.pixels + y * job.pitch;

		for(int x=0; x<job.w; x++) {
			Frequency f = m_view.freq.from + (m_view.freq.to - m_view.freq.from) * x / job.w;
			int bin = fft_w * f;
			int db = (bin > 0 && bin < fft_w) ? fft_out[bin] : -100;
			int db_min = -100;
			int db_max = -50;
			int intensity = std::clamp(255 * (db - db_min) / (db_max - db_min), 0, 255);
			*p++ = v | (uint32_t)(intensity << 24);
		}
	}

	m_result_queue.push(job);
}


void WidgetWaterfall2::gen_waterfall(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	// wait for workers
	
	while(m_jobs_in_flight > 0) {
		Job job;
		if(m_result_queue.pop(job, true)) {
			m_jobs_in_flight--;
		}
	}

	// render previous results
	
	SDL_FRect dst;
	SDL_RectToFRect(&r, &dst);
	for(auto &ch : m_channels) {
		if(ch.tex) {
			SDL_UnlockTexture(ch.tex);
			SDL_RenderTexture(rend, ch.tex, nullptr, &dst);
		}
	}

	// allocate channel textures
	allocate_channels(rend, stream.channel_count(), r.w, r.h);

	// queue jobs

	size_t stride = 0;
	size_t frames_avail = 0;
	Sample *data = stream.peek(&stride, &frames_avail);
		
	for(int ch : m_channel_map.enabled_channels()) {
	
		SDL_Color col = m_channel_map.ch_color(ch);

		uint32_t *pixels;
		int pitch;
		SDL_LockTexture(m_channels[ch].tex, nullptr, (void **)&pixels, &pitch);

		Time t = m_view.time.from;
		Time dt = (m_view.time.to - m_view.time.from) / r.h;
		ssize_t idx = ceil(stream.sample_rate() * t - m_view.window.size / 2) * stride + ch;
		ssize_t didx = ceil(stream.sample_rate() * dt) * stride;

		Job job;
		job.cmd = JobCmd::Gen;
		job.data = data;
		job.data_stride = stride;
		job.channel = &m_channels[ch];
		job.w = r.w;
		job.h = r.h;
		job.pixels = pixels;
		job.pitch = pitch / 4;
		job.idx = idx;
		job.didx = didx;
		job.idx_max = frames_avail * stride;
		job.v = *(uint32_t *)&col & 0x00FFFFFF;

		if(0) {
			job_run_gen(job);
		} else {
			m_job_queue.push(job);
			m_jobs_in_flight++;
		}
	}

}


void WidgetWaterfall2::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	
	size_t stride = 0;
	size_t frames_avail = 0;
	Sample *data = stream.peek(&stride, &frames_avail);
	
	ImGui::SameLine();
	ImGui::ToggleButton("A##pproximate FFT", &m_fft_approximate);

	if(ImGui::IsWindowFocused()) {

		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		float f = m_view.freq.cursor * stream.sample_rate() * 0.5f;
		char note[32];
		freq_to_note(f, note, sizeof(note));
		ImGui::Text("f=%.6gHz %s", f, note);

		if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.zoom_t(ImGui::GetIO().MouseDelta.y);
				m_view.zoom_freq(ImGui::GetIO().MouseDelta.x);
			} else {
				m_view.pan_freq(-ImGui::GetIO().MouseDelta.x / r.w);
				m_view.pan_t(ImGui::GetIO().MouseDelta.y / r.w);
			}
		}


		if(ImGui::IsKeyPressed(ImGuiKey_A)) {
			m_view.freq.from = 0.0f;
			m_view.freq.to = 1.0;
			m_view.time.from = 0;
			m_view.time.to   = frames_avail / stream.sample_rate();
		}

		if(ImGui::IsMouseInRect(r)) {

			auto pos = ImGui::GetIO().MousePos;

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				stream.player.seek(m_view.y_to_t(pos.y, r));
			}

			if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
				m_view.freq.cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
				m_view.time.cursor += m_view.dy_to_dt(ImGui::GetIO().MouseDelta.y, r) * 0.1;
			} else {
				m_view.freq.cursor = m_view.x_to_freq(pos.x, r);
				m_view.time.cursor = m_view.y_to_t(pos.y, r);
			}
		}
	}

	gen_waterfall(stream, rend, r);

	// darken filtered out area
	float f_lp, f_hp;
	stream.player.filter_get(f_lp, f_hp);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_FRect filt_rect;
	filt_rect.y = r.y;
	filt_rect.h = r.h;
	int x_hp = m_view.freq_to_x(f_hp, r);
	filt_rect.x = r.x;
	filt_rect.w = x_hp - r.x;
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 128);
	SDL_RenderFillRect(rend, &filt_rect);
	int x_lp = m_view.freq_to_x(f_lp, r);
	filt_rect.x = x_lp;
	filt_rect.w = r.x + r.w - x_lp;
	SDL_RenderFillRect(rend, &filt_rect);

	// selection
	if(m_view.time.sel_from != m_view.time.sel_to) {
		float sx_from = m_view.t_to_y(m_view.time.sel_from, r);
		float sx_to   = m_view.t_to_y(m_view.time.sel_to,   r);
		SDL_SetRenderDrawColor(rend, 128, 128, 255, 64);
		SDL_FRect sr = { (float)r.x, sx_from, (float)r.w, sx_to - sx_from };
		SDL_RenderFillRect(rend, &sr);
	}
	
	// cursors
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_ADD);
	int cx = m_view.freq_to_x(m_view.freq.cursor, r);
	int cy = m_view.t_to_y(m_view.time.cursor, r);
	vcursor(rend, r, cy, false);
	hcursor(rend, r, cx, false);
	vcursor(rend, r, m_view.t_to_y(m_view.time.playpos, r), true);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

}


REGISTER_WIDGET(WidgetWaterfall2,
	.name = "waterfall2",
	.description = "FFT spectrum waterfall",
	.hotkey = ImGuiKey_F12,
	.flags = Widget::Info::Flags::ShowChannelMap | 
	         Widget::Info::Flags::ShowLock |
			 Widget::Info::Flags::ShowWindowSize |
			 Widget::Info::Flags::ShowWindowType,
);
