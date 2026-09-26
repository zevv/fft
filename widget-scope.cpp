
#include <string>
#include <math.h>
#include <assert.h>
#include <algorithm>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "misc.hpp"
#include "widgetregistry.hpp"
#include "style.hpp"

// Triggered oscilloscope. Sweeps are aligned on a level/slope trigger and
// accumulated in a per-channel phosphor density buffer. Overlapping sweeps add
// up, so the brightness of a pixel is the dwell time of the trace.
//
// The time axis is widget-local: t=0 is the trigger point, m_view.time.from is
// the pre-trigger part and m_view.time.to the post-trigger part. Timebase and
// trigger position therefore use the normal pan and zoom controls, and the
// vertical offset and gain use the normal amplitude controls.

static const int k_grid_div_x = 10;
static const int k_grid_div_y = 8;
static const int k_sweep_sample_budget = 2000000;
static const double k_auto_timeout = 0.05;
static const double k_span_max = 10.0;
static const double k_span_min_samples = 8.0;


class WidgetScope : public Widget {

public:
	WidgetScope(Widget::Info &info);
	~WidgetScope() override;

private:

	enum Mode { ModeAuto, ModeNormal, ModeSingle };

	void do_load(ConfigReader::Node *node) override;
	void do_save(ConfigWriter &cfg) override;
	void do_copy(Widget *w) override;
	void do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r) override;
	bool do_handle_input(Stream &stream, SDL_Rect &r) override;

	void draw_ui(Stream &stream);
	void draw_markers(SDL_Renderer *rend, SDL_Rect &r);
	void draw_readout(SDL_Rect &r);
	void allocate(SDL_Renderer *rend, int w, int h, size_t nch);
	void sweep(Sample *data, size_t stride, ssize_t base, ssize_t f_trig,
			ssize_t n_pre, ssize_t n_post);
	void resolve(SDL_Renderer *rend, SDL_Rect &r, float decay);
	void autoscale(Stream &stream);
	void level_50pct(Stream &stream);
	void clamp_span();
	void set_span(double s);

	double span() { return m_view.time.to - m_view.time.from; }
	double span_div() { return span() / k_grid_div_x; }
	double volt_div() { return (m_view.amplitude.to - m_view.amplitude.from) / k_grid_div_y; }

	// front panel
	int m_mode{ModeAuto};
	bool m_running{true};
	bool m_rising{true};
	double m_level{0.0};      // trigger level, in view amplitude units
	double m_hyst{0.1};       // trigger hysteresis, in vertical divisions
	double m_holdoff{0.0};    // s
	double m_persist{0.25};   // phosphor time constant, s, 0 = infinite
	double m_intensity{1.0};
	int m_trig_ch{0};

	// phosphor
	SDL_Texture *m_tex{};
	int m_w{}, m_h{};
	size_t m_nch{};
	std::vector<float> m_dens;
	std::vector<uint32_t> m_pix;

	// trigger state
	ssize_t m_next_scan{0};
	ssize_t m_f_last_trig{-1};
	bool m_armed{false};
	double m_t_last_draw{};
	double m_t_last_trig{};  // last real trigger
	double m_t_last_sweep{};  // last sweep drawn, trigger or free run
	double m_freq{};
	int m_sweeps{};
	bool m_force{false};
	bool m_pending_50pct{false};
	Samplerate m_srate{};
};


WidgetScope::WidgetScope(Widget::Info &info)
	: Widget(info)
{
	m_view_config.x = View::Axis::Time;
	m_view_config.y = View::Axis::Amplitude;
	m_view.lock = false;
	m_time_local = true;
	m_view.time.from = -0.005;
	m_view.time.to = +0.005;
}


WidgetScope::~WidgetScope()
{
	if(m_tex) SDL_DestroyTexture(m_tex);
}


void WidgetScope::do_load(ConfigReader::Node *node)
{
	node->read("mode", m_mode);
	node->read("running", m_running);
	node->read("rising", m_rising);
	node->read("level", m_level);
	node->read("hysteresis_div", m_hyst);
	node->read("holdoff", m_holdoff);
	node->read("persist", m_persist);
	node->read("intensity", m_intensity);
	node->read("trig_ch", m_trig_ch);
}


void WidgetScope::do_save(ConfigWriter &cw)
{
	cw.write("mode", m_mode);
	cw.write("running", m_running);
	cw.write("rising", m_rising);
	cw.write("level", m_level);
	cw.write("hysteresis_div", m_hyst);
	cw.write("holdoff", m_holdoff);
	cw.write("persist", m_persist);
	cw.write("intensity", m_intensity);
	cw.write("trig_ch", m_trig_ch);
}


void WidgetScope::do_copy(Widget *w)
{
	WidgetScope *s = dynamic_cast<WidgetScope *>(w);
	s->m_mode = m_mode;
	s->m_running = m_running;
	s->m_rising = m_rising;
	s->m_level = m_level;
	s->m_hyst = m_hyst;
	s->m_holdoff = m_holdoff;
	s->m_persist = m_persist;
	s->m_intensity = m_intensity;
	s->m_trig_ch = m_trig_ch;
}


// Set the sweep length, keep the trigger at the same fraction of the screen.

void WidgetScope::set_span(double s)
{
	double s0 = span();
	double p = (s0 > 0.0) ? -m_view.time.from / s0 : 0.5;
	p = std::clamp(p, 0.0, 1.0);
	m_view.time.from = -p * s;
	m_view.time.to = s - p * s;
}


// The sweep can not be shorter than a handful of samples: below that there is
// nothing left to draw between two samples.

void WidgetScope::clamp_span()
{
	if(m_srate <= 0.0) return;

	// heal a view that carries absolute stream time, from an old config or from
	// a widget copy: the trigger must stay on or near the screen
	if(m_view.time.from > span() || m_view.time.to < -span()) {
		m_view.time.from = -0.005;
		m_view.time.to = +0.005;
	}

	double s = span();
	double s_min = k_span_min_samples / m_srate;
	if(s < s_min) set_span(s_min);
	if(s > k_span_max) set_span(k_span_max);
}


// ---------------------------------------------------------------------------
// phosphor accumulation
// ---------------------------------------------------------------------------

static inline void splat(float *p, int w, int h, float x, float y, float q)
{
	int xi = (int)floorf(x);
	int yi = (int)floorf(y);
	if(xi < 0 || yi < 0 || xi >= w-1 || yi >= h-1) return;
	float fx = x - xi;
	float fy = y - yi;
	float *o = p + yi * w + xi;
	o[0]     += q * (1-fx) * (1-fy);
	o[1]     += q * (  fx) * (1-fy);
	o[w]     += q * (1-fx) * (  fy);
	o[w+1]   += q * (  fx) * (  fy);
}


// Deposit one unit of charge, spread over the segment: a segment that covers
// few pixels leaves them bright, a fast slope smears the same charge out.

static void add_seg(float *p, int w, int h, float x0, float y0, float x1, float y1)
{
	float dx = x1 - x0;
	float dy = y1 - y0;
	int n = (int)(fmaxf(fabsf(dx), fabsf(dy)) + 1.0f);
	if(n > 4096) n = 4096;
	float q = 1.0f / n;
	for(int i=0; i<n; i++) {
		float f = (float)i / n;
		splat(p, w, h, x0 + dx * f, y0 + dy * f, q);
	}
}


void WidgetScope::allocate(SDL_Renderer *rend, int w, int h, size_t nch)
{
	if(w == m_w && h == m_h && nch == m_nch) return;

	if(m_tex) SDL_DestroyTexture(m_tex);
	m_w = w;
	m_h = h;
	m_nch = nch;
	m_tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING, m_w, m_h);
	SDL_SetTextureBlendMode(m_tex, SDL_BLENDMODE_ADD);
	SDL_SetTextureScaleMode(m_tex, SDL_SCALEMODE_NEAREST);
	m_dens.assign((size_t)m_w * m_h * m_nch, 0.0f);
	m_pix.assign((size_t)m_w * m_h, 0);
}


void WidgetScope::sweep(Sample *data, size_t stride, ssize_t base, ssize_t f_trig,
		ssize_t n_pre, ssize_t n_post)
{
	double a_from = m_view.amplitude.from;
	double a_to = m_view.amplitude.to;
	if(a_to == a_from) return;

	double sx = m_w / (span() * m_srate);
	double x0 = m_w * -m_view.time.from / span();
	double sy = -(double)m_h / (a_to - a_from);
	double y0 = m_h * (1.0 + a_from / (a_to - a_from));

	for(int ch : m_channel_map.enabled_channels()) {
		if(ch >= (int)m_nch) continue;
		float *plane = &m_dens[(size_t)ch * m_w * m_h];
		float px = 0, py = 0;
		for(ssize_t f = f_trig - n_pre; f <= f_trig + n_post; f++) {
			double v = data[(f - base) * stride + ch] * (1.0 / k_sample_max);
			float x = x0 + (f - f_trig) * sx;
			float y = y0 + v * sy;
			if(f > f_trig - n_pre) add_seg(plane, m_w, m_h, px, py, x, y);
			px = x;
			py = y;
		}
	}
}


// Decay the density buffers, map them through the phosphor curve and composite
// all channels into one texture.

void WidgetScope::resolve(SDL_Renderer *rend, SDL_Rect &r, float decay)
{
	static float lut[256];
	static bool lut_done = false;
	if(!lut_done) {
		for(int i=0; i<256; i++) lut[i] = 1.0f - expf(-i / 32.0f);
		lut_done = true;
	}

	std::vector<Style::Color> col(m_nch, Style::Color{0, 0, 0, 0});
	for(int ch : m_channel_map.enabled_channels()) {
		if(ch < (int)m_nch) col[ch] = Style::channel_color(ch);
	}

	size_t n = (size_t)m_w * m_h;
	float gain = m_intensity * 32.0f;

	for(size_t i=0; i<n; i++) {
		float rr = 0, gg = 0, bb = 0;
		for(size_t ch=0; ch<m_nch; ch++) {
			float d = m_dens[ch * n + i] * decay;
			m_dens[ch * n + i] = d;
			if(d < 1e-4f) continue;
			int idx = (int)(d * gain);
			float a = lut[idx < 255 ? idx : 255];
			rr += col[ch].r * a;
			gg += col[ch].g * a;
			bb += col[ch].b * a;
		}
		uint32_t cr = (uint32_t)(std::min(rr, 1.0f) * 255);
		uint32_t cg = (uint32_t)(std::min(gg, 1.0f) * 255);
		uint32_t cb = (uint32_t)(std::min(bb, 1.0f) * 255);
		m_pix[i] = 0xff000000 | (cb << 16) | (cg << 8) | cr;
	}

	SDL_UpdateTexture(m_tex, nullptr, m_pix.data(), m_w * 4);
	SDL_FRect dst = { (float)r.x, (float)r.y, (float)m_w, (float)m_h };
	SDL_RenderTexture(rend, m_tex, nullptr, &dst);
}


// ---------------------------------------------------------------------------
// decoration
// ---------------------------------------------------------------------------

static void draw_arrow(SDL_Renderer *rend, float x, float y, bool horizontal, Style::Color col)
{
	SDL_Vertex v[3];
	if(horizontal) {
		v[0].position = { x, y - 5 };
		v[1].position = { x, y + 5 };
		v[2].position = { x + 7, y };
	} else {
		v[0].position = { x - 5, y };
		v[1].position = { x + 5, y };
		v[2].position = { x, y + 7 };
	}
	for(int i=0; i<3; i++) v[i].color = col;
	int idx[] = { 0, 1, 2 };
	SDL_RenderGeometry(rend, nullptr, v, 3, idx, 3);
}


void WidgetScope::draw_markers(SDL_Renderer *rend, SDL_Rect &r)
{
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	Style::Color col = Style::channel_color(m_trig_ch);

	// trigger level: dashed line over the plot, arrow on the left edge
	float y = m_view.from_amplitude(m_view_config, r, m_level);
	if(y > r.y && y < r.y + r.h) {
		SDL_SetRenderDrawColor(rend, col.r * 255, col.g * 255, col.b * 255, 96);
		for(int x=r.x; x<r.x+r.w; x+=8) SDL_RenderLine(rend, x, y, x + 4, y);
		draw_arrow(rend, r.x, y, true, col);
	}

	// trigger point, t=0
	float x = m_view.from_t(m_view_config, r, 0.0);
	if(x > r.x && x < r.x + r.w) {
		SDL_SetRenderDrawColor(rend, col.r * 255, col.g * 255, col.b * 255, 96);
		SDL_RenderLine(rend, x, r.y, x, r.y + r.h);
		draw_arrow(rend, x, r.y, false, col);
	}
}


void WidgetScope::draw_readout(SDL_Rect &r)
{
	static const char *mode_name[] = { "AUTO", "NORM", "SINGLE" };
	char b_span[32], b_volt[32], b_freq[32];
	humanize(span_div(), b_span, sizeof(b_span));
	humanize(volt_div(), b_volt, sizeof(b_volt));

	if(m_freq > 0.0) {
		humanize(m_freq, b_freq, sizeof(b_freq));
	} else {
		snprintf(b_freq, sizeof(b_freq), "?");
	}

	double now = hirestime();
	bool trigd = (now - m_t_last_trig) < 0.5;
	bool swept = (now - m_t_last_sweep) < 0.5;

	// a free running trace is not a triggered one, say so
	const char *state = "WAIT";
	if(!m_running) state = "STOP";
	else if(trigd) state = "TRIG'D";
	else if(swept) state = "FREE";

	char buf[192];
	snprintf(buf, sizeof(buf), "%ss/div  %s/div  ch%d %s %.4f  %s %s  f=%sHz  %d sw",
			b_span, b_volt, m_trig_ch, m_rising ? "/" : "\\", m_level,
			mode_name[m_mode], state, b_freq, m_sweeps);

	ImDrawList *dl = ImGui::GetWindowDrawList();
	dl->AddText(ImVec2(r.x + 4, r.y + r.h - 16), 0xFFA0A0A0, buf);
}


void WidgetScope::draw_ui(Stream &stream)
{
	ImGui::SameLine();
	if(ImGui::Button(m_running ? "STOP" : "RUN")) m_running = !m_running;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::Combo("##mode", &m_mode, "AUTO\0NORM\0SINGLE\0");

	char chans[128] = {};
	size_t o = 0;
	for(size_t ch=0; ch<stream.channel_count() && o<sizeof(chans)-8; ch++) {
		o += snprintf(chans + o, sizeof(chans) - o, "ch%zu", ch) + 1;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	ImGui::Combo("##trigch", &m_trig_ch, chans);

	ImGui::SameLine();
	if(ImGui::Button(m_rising ? "/##slope" : "\\##slope")) m_rising = !m_rising;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::DragDouble("##level", &m_level, 0.001, -1.0, 1.0, "%.3f");

	ImGui::SameLine();
	if(ImGui::Button("50%")) m_pending_50pct = true;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(90);
	ImGui::SliderDouble("##persist", &m_persist, 0.0, 5.0, "%.2fs pers",
			ImGuiSliderFlags_Logarithmic);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(90);
	ImGui::SliderDouble("##intensity", &m_intensity, 0.01, 20.0, "%.2f int",
			ImGuiSliderFlags_Logarithmic);
}


void WidgetScope::do_draw(Stream &stream, SDL_Renderer *rend, SDL_Rect &r)
{
	m_srate = stream.sample_rate();
	draw_ui(stream);
	if(m_srate <= 0.0 || r.w < 16 || r.h < 16) return;
	if(m_trig_ch >= (int)stream.channel_count()) m_trig_ch = 0;

	m_view.lock = false;
	clamp_span();
	allocate(rend, r.w, r.h, stream.channel_count());

	size_t stride = 0, avail = 0;
	Sample *data = stream.peek(&stride, &avail);
	ssize_t head = stream.head_frame();
	ssize_t base = head - (ssize_t)avail;

	if(m_pending_50pct) {
		level_50pct(stream);
		m_pending_50pct = false;
	}

	double now = hirestime();
	double dt = std::clamp(now - m_t_last_draw, 0.0, 1.0);
	m_t_last_draw = now;

	ssize_t n_pre = -m_view.time.from * m_srate;
	ssize_t n_post = m_view.time.to * m_srate;
	ssize_t n_total = std::max<ssize_t>(2, n_pre + n_post + 1);

	ssize_t f_lo = base + n_pre + 1;
	ssize_t f_hi = head - n_post - 1;
	if(m_next_scan < f_lo) m_next_scan = f_lo;

	int budget = std::max<ssize_t>(1, k_sweep_sample_budget / n_total);
	m_sweeps = 0;

	if(m_running && f_hi > m_next_scan) {

		double lvl = m_level * k_sample_max;
		double hyst = fabs(m_hyst) * volt_div() * k_sample_max;
		double v_hi = lvl + hyst;
		double v_lo = lvl - hyst;
		ssize_t hold = std::max<ssize_t>(1, m_holdoff * m_srate);

		ssize_t f = m_next_scan;
		while(f < f_hi) {
			Sample v = data[(f - base) * stride + m_trig_ch];
			bool fire = false;
			if(m_rising) {
				if(!m_armed) m_armed = (v < v_lo);
				else if(v >= v_hi) fire = true;
			} else {
				if(!m_armed) m_armed = (v > v_hi);
				else if(v <= v_lo) fire = true;
			}
			if(!fire) {
				f++;
				continue;
			}

			m_armed = false;
			sweep(data, stride, base, f, n_pre, n_post);
			m_sweeps++;
			m_t_last_trig = now;
			m_t_last_sweep = now;
			if(m_f_last_trig > 0 && f > m_f_last_trig) {
				double freq = m_srate / (double)(f - m_f_last_trig);
				m_freq = (m_freq > 0.0) ? m_freq * 0.7 + freq * 0.3 : freq;
			}
			m_f_last_trig = f;
			f += hold;
			if(m_mode == ModeSingle) {
				m_running = false;
				break;
			}
			if(m_sweeps >= budget) break;
		}
		m_next_scan = std::max(m_next_scan, f);
	}

	// auto mode free runs when no trigger comes in: once the trigger is lost it
	// sweeps on every redraw, so the trace stays as bright as a triggered one
	if(m_running && m_sweeps == 0 && m_mode == ModeAuto &&
	   now - m_t_last_trig > k_auto_timeout && f_hi > f_lo) {
		sweep(data, stride, base, f_hi - 1, n_pre, n_post);
		m_sweeps++;
		m_t_last_sweep = now;
		m_freq = 0.0;
		m_next_scan = f_hi;
	}

	// single shot forced by hand
	if(m_force && f_hi > f_lo) {
		sweep(data, stride, base, f_hi - 1, n_pre, n_post);
		m_sweeps++;
		m_t_last_sweep = now;
		m_next_scan = f_hi;
		m_force = false;
	}

	float decay = 1.0f;
	if(m_running && m_persist > 0.0) decay = expf(-dt / m_persist);

	resolve(rend, r, decay);
	draw_markers(rend, r);
	draw_readout(r);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
}


// ---------------------------------------------------------------------------
// front panel behavior
// ---------------------------------------------------------------------------

void WidgetScope::level_50pct(Stream &stream)
{
	size_t stride = 0, avail = 0;
	Sample *data = stream.peek(&stride, &avail);
	if(avail == 0) return;
	size_t n = std::min<size_t>(avail, m_srate * 0.25);
	Sample v_min = data[(avail - n) * stride + m_trig_ch];
	Sample v_max = v_min;
	for(size_t i=avail-n; i<avail; i++) {
		Sample v = data[i * stride + m_trig_ch];
		v_min = std::min(v_min, v);
		v_max = std::max(v_max, v);
	}
	m_level = 0.5 * (v_min + v_max) / (double)k_sample_max;
}


void WidgetScope::autoscale(Stream &stream)
{
	size_t stride = 0, avail = 0;
	Sample *data = stream.peek(&stride, &avail);
	if(avail == 0) return;
	size_t n = std::min<size_t>(avail, m_srate * 0.25);
	size_t i0 = avail - n;

	Sample v_min = data[i0 * stride + m_trig_ch];
	Sample v_max = v_min;
	for(size_t i=i0; i<avail; i++) {
		Sample v = data[i * stride + m_trig_ch];
		v_min = std::min(v_min, v);
		v_max = std::max(v_max, v);
	}
	if(v_max <= v_min) return;

	double mid = 0.5 * (v_min + v_max);
	double pp = v_max - v_min;
	m_level = mid / k_sample_max;
	m_view.amplitude.from = (mid - pp * 0.8) / k_sample_max;
	m_view.amplitude.to   = (mid + pp * 0.8) / k_sample_max;

	// mean period between upward crossings of the midpoint
	double hyst = pp * 0.1;
	bool armed = false;
	ssize_t f_first = -1, f_last = -1;
	int count = 0;
	for(size_t i=i0; i<avail; i++) {
		double v = data[i * stride + m_trig_ch];
		if(!armed) {
			armed = (v < mid - hyst);
		} else if(v >= mid + hyst) {
			armed = false;
			if(f_first < 0) f_first = i; else { f_last = i; count++; }
		}
	}
	if(count > 0 && f_last > f_first) {
		double period = (f_last - f_first) / (double)count / m_srate;
		set_span(std::clamp(period * 3.0, 8.0 / m_srate, k_span_max));
	}

	m_mode = ModeAuto;
	m_running = true;
}


bool WidgetScope::do_handle_input(Stream &stream, SDL_Rect &r)
{
	ImGuiIO &io = ImGui::GetIO();
	auto pos = io.MousePos;

	if(ImGui::IsKeyPressed(ImGuiKey_R)) m_running = !m_running;
	if(ImGui::IsKeyPressed(ImGuiKey_S)) { m_mode = ModeSingle; m_running = true; }
	if(ImGui::IsKeyPressed(ImGuiKey_N)) m_mode = ModeNormal;
	if(ImGui::IsKeyPressed(ImGuiKey_T)) { m_force = true; m_running = true; }
	if(ImGui::IsKeyPressed(ImGuiKey_5)) m_pending_50pct = true;
	if(ImGui::IsKeyPressed(ImGuiKey_E)) m_dens.assign(m_dens.size(), 0.0f);

	// key 'A': autoscale, instead of the plain view reset
	if(ImGui::IsKeyPressed(ImGuiKey_A)) {
		autoscale(stream);
		return true;
	}

	// mouse LMB: set the trigger point, level and position in one go
	if(ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseInRect(r)) {
		double a_from = m_view.amplitude.from;
		double a_to = m_view.amplitude.to;
		m_level = std::clamp(a_from + (a_to - a_from) * (1.0 - (pos.y - r.y) / r.h), -1.0, 1.0);
		double t = m_view.time.from + span() * (pos.x - r.x) / r.w;
		m_view.time.from -= t;
		m_view.time.to -= t;
		return true;
	}

	// everything else is the standard pan and zoom: time on x, amplitude on y
	return false;
}


REGISTER_WIDGET(WidgetScope,
	.name = "scope",
	.description = "Triggered oscilloscope",
	.hotkey = ImGuiKey_F7,
	.flags = Widget::Info::Flags::ShowChannelMap,
);
