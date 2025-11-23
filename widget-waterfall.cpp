#include <thread>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <array>

#include "misc.hpp"
#include "widgetregistry.hpp"
#include "fft.hpp"
#include "queue.hpp"
#include "histogram.hpp"
#include "style.hpp"


class WidgetWaterfall : public Widget {
public:
	WidgetWaterfall(Widget::Info &info);
	~WidgetWaterfall() override;

private:

	typedef Range<double> Aperture;

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
		int col_count;
		Range<int> row;
		Range<Frequency> f;
		uint32_t *pixels;
		uint32_t color;
		Samplerate srate;
		Time t_start;
		Time dt_row;
		size_t ch;
		ssize_t idx_max;
		ssize_t idx_stepsize;
		Aperture aperture;
	};

	struct Result {
		Histogram<int8_t> hist{256, -126, 127};
	};

	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;
	void gen_waterfall(Stream &stream, SDL_Renderer *rend, SDL_Rect &r);
	void work(Worker &w);
	void allocate_channels(SDL_Renderer *rend, size_t channels, int w, int h);
	void job_run_gen(Worker &worker, Job &job);

	bool m_agc{true};
	std::vector<SDL_Texture *> m_ch_tex;

	std::vector<Worker *> m_workers;
	Queue<Job> m_job_queue{32};
	Queue<Result> m_result_queue{32};
	size_t m_jobs_in_flight{0};
	bool m_rotate{false};
};



WidgetWaterfall::WidgetWaterfall(Widget::Info &info)
	: Widget(info)
{
	size_t nthreads = std::thread::hardware_concurrency();
	printf("WidgetWaterfall: starting %zu worker threads\n", nthreads);
	for(size_t i=0; i<nthreads; i++) {
		Worker *w = new Worker();
		w->id = (int)i;
		w->thread = std::thread([this, w]() {
			this->work(*w);
		});
		m_workers.push_back(w);
	}

}


WidgetWaterfall::~WidgetWaterfall()
{
	for(size_t i=0; i<m_workers.size(); i++) {
		Job job;
		job.cmd = JobCmd::Stop;
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


void WidgetWaterfall::do_load(ConfigReader::Node *node)
{
	auto *wnode = node->find("waterfall");
	wnode->read("rotate", m_rotate);
	wnode->read("agc", m_agc);
}


void WidgetWaterfall::do_save(ConfigWriter &cw)
{
	cw.push("waterfall");
	cw.write("rotate", m_rotate);
	cw.write("agc", m_agc);
	cw.pop();
}


void WidgetWaterfall::do_copy(Widget *w)
{
	WidgetWaterfall *wh = dynamic_cast<WidgetWaterfall*>(w);
	wh->m_rotate = m_rotate;
}


void WidgetWaterfall::work(Worker &w)
{
	while(true) {
		Job job;
		m_job_queue.pop(job, true);

		if(job.cmd == JobCmd::Stop) {
			break;
		}

		if(job.cmd == JobCmd::Gen) {
			bitline("+ t%d.job.gen", hirestime(), w.id);
			job_run_gen(w, job);
			bitline("- t%d.job.gen", hirestime(), w.id);
		}
	}
}


void WidgetWaterfall::allocate_channels(SDL_Renderer *rend, size_t channels, int w, int h)
{
	if(m_rotate) std::swap(w, h);
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
		w->fft.set_approximate(true);
	}
}


void WidgetWaterfall::job_run_gen(Worker &worker, Job &job)
{
	uint32_t color = job.color;
	int fft_w = worker.fft.out_size();

	Result res;

	Time t = job.t_start;
	uint32_t *p = job.pixels;
	for(int row=job.row.min; row<job.row.max; row++) {
		ssize_t idx = (job.srate * t - m_view.window.size / 2) * job.data_stride;
		idx = (idx / job.idx_stepsize) * job.idx_stepsize;
		if(idx >= 0 && idx < job.idx_max) {
			auto fft_out = worker.fft.run(&job.data[idx + job.ch], job.data_stride);
			for(int col=0; col<job.col_count; col++) {
				Frequency f = job.f.min + (job.f.max - job.f.min) * col / job.col_count;
				if(f >= 0 && f <= 1.0) {
					double db = tabread2(fft_out, f, -127.0f);
					res.hist.add(db);
					uint32_t alpha = std::clamp(255 * (db - job.aperture.min) / (job.aperture.max - job.aperture.min), 0.0, 255.0);
					*p = color | (alpha << 24);
				}
				p ++;
			}
		} else {
			p += job.col_count;
		}
		t += job.dt_row;
	}

	m_result_queue.push(res);
}


void WidgetWaterfall::gen_waterfall(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	// wait for workers

	Histogram<int8_t> hist(256, -128, 127);

	while(m_jobs_in_flight > 0) {
		Result res;
		m_result_queue.pop(res, true);
		hist.add(res.hist);
		m_jobs_in_flight--;
	}
			
	if(m_agc) {
		m_view.aperture.from = hist.find_percentile(0.01);
		m_view.aperture.to = hist.find_percentile(1.00);
		m_view.aperture.to = std::max(m_view.aperture.to, m_view.aperture.from + 10.0f);
	}

	// render previous results
	
	SDL_FRect dst;
	SDL_RectToFRect(&r, &dst);
	for(auto &tex : m_ch_tex) {
		if(tex) {
			SDL_FPoint tl, tr, bl;
			if(m_rotate) {
				tl = {dst.x, dst.y + dst.h};
				tr = {dst.x, dst.y};
				bl = {dst.x + dst.w, dst.y + dst.h};
			} else {
				tl = {dst.x, dst.y};
				tr = {dst.x + dst.w, dst.y};
				bl = {dst.x, dst.y + dst.h};
			}
			SDL_UnlockTexture(tex);
			SDL_RenderTextureAffine(rend, tex, nullptr, &tl, &tr, &bl);
		}
	}

	// allocate channel textures

	allocate_channels(rend, stream.channel_count(), r.w, r.h);

	// constriant idx to always be a muiltiple of indices-per-pixel to
	// avoid aliasing artifacts when panning

	double idx_from = m_view.time.from * stream.sample_rate();
	double idx_to = m_view.time.to * stream.sample_rate();
	ssize_t idx_stepsize = ceil((idx_to - idx_from) / (m_rotate ? r.w : r.h)) * stream.channel_count();


	// queue jobs

	size_t stride = 0;
	size_t frames_avail = 0;
	Sample *data = stream.peek(&stride, &frames_avail);
	
	for(size_t ch=0; ch<stream.channel_count(); ch++) {

		SDL_Color col = Style::channel_color(ch);

		uint32_t *pixels;
		int pitch;
		SDL_LockTexture(m_ch_tex[ch], nullptr, (void **)&pixels, &pitch);

		memset(pixels, 0, r.w * r.h * 4);

		if(m_channel_map.is_channel_enabled(ch)) {

			int row_count = m_rotate ? r.w : r.h;
			int col_count = m_rotate ? r.h : r.w;

			Time dt_row = (m_view.time.to - m_view.time.from) / row_count;

			for(int row=0; row<row_count; row+=128) {

				Job job;
				job.cmd = JobCmd::Gen;
				job.data = data;
				job.data_stride = stride;
				job.col_count = col_count;
				job.row.min = row;
				job.row.max = std::min(row + 128, row_count);
				job.f.min = m_view.freq.from;
				job.f.max = m_view.freq.to;
				job.pixels = pixels + job.row.min * (pitch / 4);
				job.srate = stream.sample_rate();
				job.t_start = m_view.time.from + dt_row * row;
				job.dt_row = dt_row;
				job.ch = ch;
				job.idx_max = frames_avail * stride;
				job.color = *(uint32_t *)&col & 0x00FFFFFF;
				job.aperture.min = m_view.aperture.from;
				job.aperture.max = m_view.aperture.to;
				job.idx_stepsize = idx_stepsize;

				m_job_queue.push(job);
				m_jobs_in_flight++;
			}
		}

	}
}


void WidgetWaterfall::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	if(m_rotate) {
		m_view_config.x = View::Axis::Time;
		m_view_config.y = View::Axis::Frequency;
	} else {
		m_view_config.x = View::Axis::Frequency;
		m_view_config.y = View::Axis::Time;
	}

	
	size_t stride = 0;
	size_t frames_avail = 0;
	Sample *data = stream.peek(&stride, &frames_avail);
	
	ImGui::SameLine();
	ImGui::ToggleButton("AGC", &m_agc);

	if(!m_agc) {
		float aperture_center = (m_view.aperture.from + m_view.aperture.to) * 0.5;
		float aperture_range = m_view.aperture.to - m_view.aperture.from;
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db center", &aperture_center, 0.0f, -100.0f, "%.1f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db range", &aperture_range, 100.0f, 0.0f, "%.1f");
		m_view.aperture.from = aperture_center - aperture_range * 0.5f;
		m_view.aperture.to = aperture_center + aperture_range * 0.5f;
	}

	if(ImGui::IsWindowFocused()) {
			
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		char fbuf[64];
		float f = m_view.freq.cursor * stream.sample_rate() * 0.5f;
		humanize(f, fbuf, sizeof(fbuf));
		char note[32];
		freq_to_note(f, note, sizeof(note));
		ImGui::TextShadow("%sHz / %s / %+.0f..%+.0f dB", fbuf, note, m_view.aperture.from, m_view.aperture.to);

		if(ImGui::IsKeyPressed(ImGuiKey_R)) {
			m_rotate = !m_rotate;
		}
	}
	gen_waterfall(stream, rend, r);
}


REGISTER_WIDGET(WidgetWaterfall,
	.name = "waterfall",
	.description = "FFT spectrum waterfall",
	.hotkey = ImGuiKey_F3,
	.flags = Widget::Info::Flags::ShowChannelMap | 
	         Widget::Info::Flags::ShowLock |
			 Widget::Info::Flags::ShowWindowSize |
			 Widget::Info::Flags::ShowWindowType,
);
