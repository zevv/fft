#include <thread>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <array>

#include "misc.hpp"
#include "widgetregistry.hpp"
#include "fft.hpp"
#include "queue.hpp"
#include "histogram.hpp"


class WidgetWaterfall : public Widget {
public:
	WidgetWaterfall(Widget::Info &info);
	~WidgetWaterfall() override;

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
		int cols;
		Range<int> row;
		Range<Frequency> f;
		uint32_t *pixels;
		size_t dp_col;
		size_t dp_row;
		uint32_t v;
		Samplerate srate;
		Time t_start;
		Time dt_row;
		size_t ch;
		ssize_t idx_max;
		Aperture aperture;
	};

	struct Result {
		Histogram<int8_t> hist{256, -128, 127};
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
	Aperture m_aperture{-127, -0};
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
}


void WidgetWaterfall::do_save(ConfigWriter &cw)
{
	cw.push("waterfall");
	cw.write("rotate", m_rotate);
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
	uint32_t v = job.v;
	int fft_w = worker.fft.out_size();

	if(job.aperture.max == job.aperture.min) {
		job.aperture.max++;
	}
	
	Result res;

	Time t = job.t_start;
	for(int row=job.row.min; row<job.row.max; row++) {
		ssize_t idx = ceil(job.srate * t - m_view.window.size / 2) * job.data_stride + job.ch;
		if(idx >=0 && idx < job.idx_max) {
			auto fft_out = worker.fft.run(&job.data[idx], job.data_stride);
			uint32_t *p = job.pixels + row * job.dp_row;
			for(int col=0; col<job.cols; col++) {
				Frequency f = job.f.min + (job.f.max - job.f.min) * col / job.cols;
				if(f >=0 && f <= 1.0) {
					int db = tabread2(fft_out, f, (int8_t)-127);
					res.hist.add(db);
					uint32_t alpha = std::clamp(255 * (db - job.aperture.min) / (job.aperture.max - job.aperture.min), 0, 255);
					*p = v | (alpha << 24);
				}
				p += job.dp_col;
			}
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
		m_aperture.min = hist.find_percentile(0.05);
		m_aperture.max = hist.find_percentile(1.00) + 10.0;
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

			int row_count = m_rotate ? r.w : r.h;
			int col_count = m_rotate ? r.h : r.w;

			Time dt_row = (m_view.time.to - m_view.time.from) / row_count;

			for(int row=0; row<row_count; row+=128) {

				Job job;
				job.cmd = JobCmd::Gen;
				job.data = data;
				job.data_stride = stride;
				job.cols = col_count;
				job.row.min = row;
				job.row.max = std::min(row + 128, row_count);
				job.f.min = m_view.freq.from;
				job.f.max = m_view.freq.to;
				job.pixels = pixels;
				job.dp_col = 1;
				job.dp_row = pitch / 4;
				job.srate = stream.sample_rate();
				job.t_start = m_view.time.from + dt_row * row;
				job.dt_row = dt_row;
				job.ch = ch;
				job.idx_max = frames_avail * stride;
				job.v = *(uint32_t *)&col & 0x00FFFFFF;
				job.aperture = m_aperture;

				if(m_rotate) {
					job.pixels = pixels + (r.h - 1) * (pitch / 4);
					job.dp_col = -pitch / 4;
					job.dp_row = 1;
				}

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
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db center", &m_view.aperture.center, 0.0f, -100.0f, "%.1f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##db range", &m_view.aperture.range, 100.0f, 0.0f, "%.1f");
		m_aperture.min = std::clamp((int)(m_view.aperture.center - m_view.aperture.range), -127, 0);
		m_aperture.max = std::clamp((int)(m_view.aperture.center + m_view.aperture.range), -127, 0);
	}

	if(ImGui::IsWindowFocused()) {
			
		ImGui::SetCursorPosY(r.h + ImGui::GetTextLineHeightWithSpacing());
		char fbuf[64];
		float f = m_view.freq.cursor * stream.sample_rate() * 0.5f;
		humanize(f, fbuf, sizeof(fbuf));
		char note[32];
		freq_to_note(f, note, sizeof(note));
		ImGui::TextShadow("%sHz / %s / %+d..%+d dB", fbuf, note, m_aperture.min, m_aperture.max);

		if(ImGui::IsKeyPressed(ImGuiKey_R)) {
			m_rotate = !m_rotate;
		}
	}

	gen_waterfall(stream, rend, r);

	// filter pos
	//float f_lp, f_hp;
	//stream.player.filter_get(f_lp, f_hp);

	// selection
	// if(m_view.time.sel_from != m_view.time.sel_to) {
	// 	float sx_from = m_view.t_to_y(m_view.time.sel_from, r);
	// 	float sx_to   = m_view.t_to_y(m_view.time.sel_to,   r);
	// 	SDL_SetRenderDrawColor(rend, 128, 128, 255, 64);
	// 	SDL_FRect sr = { (float)r.x, sx_from, (float)r.w, sx_to - sx_from };
	// 	SDL_RenderFillRect(rend, &sr);
	// }

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

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
