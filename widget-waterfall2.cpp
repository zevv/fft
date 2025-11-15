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

	typedef Range<int8_t> Aperture;

	struct Worker {
		int id;
		std::thread thread;
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
		int w;
		int y_from, y_to;
		Frequency f_from, f_to;
		uint32_t *pixels;
		size_t pitch;
		uint32_t v;
		ssize_t idx;
		ssize_t didx;
		ssize_t idx_max;
		Aperture aperture;
	};

	struct Result {
		Aperture aperture;
	};

	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;
	void gen_waterfall(Stream &stream, SDL_Renderer *rend, SDL_Rect &r);
	void work(Worker &w);
	void allocate_channels(SDL_Renderer *rend, size_t channels, int w, int h);
	void job_run_gen(Worker &worker, Job &job);

	bool m_agc{true};
	std::vector<SDL_Texture *> m_ch_tex;

	bool m_fft_approximate{true};

	std::vector<Worker *> m_workers;
	Queue<Job> m_job_queue{32};
	Queue<Result> m_result_queue{32};
	size_t m_jobs_in_flight{0};
	Aperture m_aperture{-120, -0};
};



WidgetWaterfall2::WidgetWaterfall2(Widget::Info &info)
	: Widget(info)
{
	size_t nthreads = std::thread::hardware_concurrency();
	for(size_t i=0; i<nthreads; i++) {
		Worker *w = new Worker();
		w->id = (int)i;
		w->thread = std::thread([this, w]() {
			this->work(*w);
		});
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
	for(auto &tex : m_ch_tex) {
		if(tex) SDL_DestroyTexture(tex);
	}
}


void WidgetWaterfall2::work(Worker &w)
{
	while(true) {
		Job job;
		m_job_queue.pop(job, true);

		if(job.cmd == JobCmd::Stop) {
			break;
		}

		if(job.cmd == JobCmd::Gen) {
			bitline("+ t%d.job.gen.%d", hirestime(), w.id, job.id);
			job_run_gen(w, job);
			bitline("- t%d.job.gen.%d", hirestime(), w.id, job.id);
		}
	}
}


void WidgetWaterfall2::allocate_channels(SDL_Renderer *rend, size_t channels, int w, int h)
{
	m_ch_tex.resize(channels);

	for(auto &tex : m_ch_tex) {
		if(!tex || tex->w != w || tex->h != h) {
			if(tex) SDL_DestroyTexture(tex);
			tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
			SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
		}
	}

	for(auto &w : m_workers) {
		w->fft.configure(m_view.window.size, m_view.window.window_type, m_view.window.window_beta);
		w->fft.set_approximate(m_fft_approximate);
	}
}


void WidgetWaterfall2::job_run_gen(Worker &worker, Job &job)
{
	ssize_t idx = job.idx;
	uint32_t v = job.v;
	int fft_w = worker.fft.out_size();

	Result res;
	res.aperture.min = INT8_MAX;
	res.aperture.max = INT8_MIN;
		

	for(int y=job.y_from; y<job.y_to; y++) {

		idx += job.didx;
		if(idx < 0) continue;
		if(idx >= job.idx_max) break;

		auto fft_out = worker.fft.run(&job.data[idx], job.data_stride);
		uint32_t *p = job.pixels + y * job.pitch;

		for(int x=0; x<job.w; x++) {
			Frequency f = job.f_from + (job.f_to - job.f_from) * x / job.w;
			int db = tabread2(fft_out, f, (int8_t)-120);

			res.aperture.min = std::min(res.aperture.min, (int8_t)db);
			res.aperture.max = std::max(res.aperture.max, (int8_t)db);

			int intensity = std::clamp(255 * (db - job.aperture.min) / (job.aperture.max - job.aperture.min), 0, 255);
			*p++ = v | (uint32_t)(intensity << 24);
		}
	}

	m_result_queue.push(res);
}


void WidgetWaterfall2::gen_waterfall(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	// wait for workers

	Aperture aperture_accum = { -120, 0 };

	while(m_jobs_in_flight > 0) {
		Result res;
		m_result_queue.pop(res, true);
		aperture_accum.min = std::max(aperture_accum.min, res.aperture.min);
		aperture_accum.max = std::min(aperture_accum.max, res.aperture.max);
		m_jobs_in_flight--;
	}

	if(m_agc) {
		m_aperture = aperture_accum;
	}

	// render previous results
	
	SDL_FRect dst;
	SDL_RectToFRect(&r, &dst);
	for(auto &tex : m_ch_tex) {
		if(tex) {
			SDL_UnlockTexture(tex);
			SDL_RenderTexture(rend, tex, nullptr, &dst);
		}
	}

	// allocate channel textures
	allocate_channels(rend, stream.channel_count(), r.w, r.h);

	// queue jobs

	size_t stride = 0;
	size_t frames_avail = 0;
	Sample *data = stream.peek(&stride, &frames_avail);
	
	for(size_t ch=0; ch<stream.channel_count(); ch++) {

		SDL_Color col = m_channel_map.ch_color(ch);

		uint32_t *pixels;
		int pitch;
		SDL_LockTexture(m_ch_tex[ch], nullptr, (void **)&pixels, &pitch);

		memset(pixels, 0, pitch * r.h);

		if(m_channel_map.is_channel_enabled(ch)) {

			for(int y=0; y<r.h; y+=128) {

				Time t = m_view.y_to_t(y + r.y, r);
				Time dt = (m_view.time.to - m_view.time.from) / r.h;
				ssize_t idx = ceil(stream.sample_rate() * t - m_view.window.size / 2) * stride + ch;
				ssize_t didx = ceil(stream.sample_rate() * dt) * stride;

				Job job;
				job.cmd = JobCmd::Gen;
				job.data = data;
				job.data_stride = stride;
				job.w = r.w;
				job.y_from = y;
				job.y_to = std::min(y + 128, r.h);
				job.f_from = m_view.freq.from;
				job.f_to = m_view.freq.to;
				job.pixels = pixels;
				job.pitch = pitch / 4;
				job.idx = idx;
				job.didx = didx;
				job.idx_max = frames_avail * stride;
				job.v = *(uint32_t *)&col & 0x00FFFFFF;
				job.aperture = m_aperture;

				m_job_queue.push(job);
				m_jobs_in_flight++;
			}
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

	ImGui::SameLine();
	ImGui::ToggleButton("AGC", &m_agc);

	if(!m_agc) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db center", &m_view.aperture.center, 0.0f, -100.0f, "%.1f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db range", &m_view.aperture.range, 100.0f, 0.0f, "%.1f");
		m_aperture.min = std::clamp((int)(m_view.aperture.center - m_view.aperture.range), -120, 0);
		m_aperture.max = std::clamp((int)(m_view.aperture.center + m_view.aperture.range), -120, 0);
	}

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

			//if(ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			//	m_view.freq.cursor += m_view.dx_to_dfreq(ImGui::GetIO().MouseDelta.x, r) * 0.1f;
			//	m_view.time.cursor += m_view.dy_to_dt(ImGui::GetIO().MouseDelta.y, r) * 0.1;
			//} else {
			//	m_view.freq.cursor = m_view.x_to_freq(pos.x, r);
			//	m_view.time.cursor = m_view.y_to_t(pos.y, r);
			//}
			m_view.freq.cursor = m_view.x_to_freq(pos.x, r);
			m_view.time.cursor = m_view.y_to_t(pos.y, r);
		}
	}

	gen_waterfall(stream, rend, r);

	// filter pos
	float f_lp, f_hp;
	stream.player.filter_get(f_lp, f_hp);
	//vcursor(rend, r, m_view.freq_to_x(f_lp, r), true);
	//vcursor(rend, r, m_view.freq_to_x(f_hp, r), true);


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
