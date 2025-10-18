
#include <stdlib.h>
#include <imgui.h>

#include "panel.hpp"
	

Panel::Panel(Widget *widget)
	: m_parent(nullptr)
	, m_widget(widget)
	, m_type(Type::Widget)
	, m_weight(1.0)
{
	char buf[16] = "";
	for(int i=0; i<15; i++) {
		char c = 'a' + (rand() % 26);
		buf[i] = c;
	}
	buf[15] = 0;
	m_title = strdup(buf);
}


Panel::Panel(Type type)
	: m_parent(nullptr)
	, m_widget(nullptr)
	, m_type(type)
	, m_weight(1.0)
	, m_kids{}
{
	char buf[16] = "";
	for(int i=0; i<15; i++) {
		char c = 'a' + (rand() % 26);
		buf[i] = c;
	}
	buf[15] = 0;
	m_title = strdup(buf);
}


Panel::~Panel()
{
	fprintf(stderr, "Deleting panel %s\n", m_title);
	free((void *)m_title);
}


void Panel::load(ConfigReader::Node *node)
{
	if(node) {

		node->read("weight", m_weight);
		
		if(const char *type = node->read_str("type")) {

			if(strcmp(type, "split_v") == 0) {
				m_type = Type::SplitV;
			}

			if(strcmp(type, "split_h") == 0) {
				m_type = Type::SplitH;
			}

			if(strcmp(type, "widget") == 0) {
				m_type = Type::Widget;
				m_widget = new Widget(Widget::Type::None);
				m_widget->load(node->find("widget"));
			}

			if(auto kids = node->find("kids")) {
				for(auto &k : kids->kids) {
					Panel *p = new Panel(Type::None);
					p->load(k.second);
					add(p);
				}
			}
		}
	}
}


void Panel::save(ConfigWriter &cw)
{
	if(m_type == Type::Widget) {
		cw.write("type", "widget");
		cw.write("weight", m_weight);
		m_widget->save(cw);
	} else if(m_type == Type::SplitH) {
		cw.write("type", "split_h");
		cw.write("weight", m_weight);
		cw.push("kids");
		for(size_t i=0; i<m_kids.size(); i++) {
			cw.push(i);
			m_kids[i]->save(cw);
			cw.pop();
		}
		cw.pop();
	} else if(m_type == Type::SplitV) {
		cw.write("type", "split_v");
		cw.write("weight", m_weight);
		cw.push("kids");
		for(size_t i=0; i<m_kids.size(); i++) {
			cw.push(i);
			m_kids[i]->save(cw);
			cw.pop();
		}
		cw.pop();
	}
}

	
void Panel::add(Panel *p, Panel *p_after)
{
	AddRequest req = { p, p_after };
	m_add_requests.push_back(req);
}


void Panel::add(Widget *w)
{
	Panel *p = new Panel(w);
	add(p);
}


void Panel::replace(Panel *kid_old, Panel *kid_new)
{
	for(size_t i=0; i<m_kids.size(); i++) {
		if(m_kids[i] == kid_old) {
			m_kids[i] = kid_new;
			kid_new->m_parent = this;
			kid_new->m_weight = kid_old->m_weight;
			kid_old->m_parent = nullptr;
		}
	}
}


void Panel::remove(Panel *kid)
{
	m_kids_remove.push_back(kid);
}


void Panel::update_kid(Panel *pk, int dx1, int dy1, int dx2, int dy2)
{
	size_t nkids = m_kids.size();

	float total_weight = 0.0f;
	for(auto &k : m_kids) {
		total_weight += k->m_weight;
	}

	if(m_type == Type::SplitH) {
		int drawable_w = m_last_w - (m_kids.size() > 0 ? m_kids.size() - 1 : 0);
		float pixels_per_weight = drawable_w > 0 ? (float)drawable_w / total_weight : 0.0f;

		for(size_t i=0; i<m_kids.size(); i++) {
			if(m_kids[i] == pk) {
				if(dx1 != 0 && i > 0) {
					Panel *pp = m_kids[i-1];
					float weight_delta = (float)dx1 / pixels_per_weight;
					pk->m_weight -= weight_delta;
					pp->m_weight += weight_delta;
				}
				if(i < (nkids - 1)) {
					Panel *pn = m_kids[i+1];
					float weight_delta = (float)dx2 / pixels_per_weight;
					pk->m_weight += weight_delta;
					pn->m_weight -= weight_delta;
				}
			}
		}
		if (m_parent) {
			m_parent->update_kid(this, 0, dy1, 0, dy2);
		}
	}

	if(m_type == Type::SplitV) {
		int drawable_h = m_last_h - (m_kids.size() > 0 ? m_kids.size() - 1 : 0);
		float pixels_per_weight = drawable_h > 0 ? (float)drawable_h / total_weight : 0.0f;

		for(size_t i=0; i<m_kids.size(); i++) {
			if(m_kids[i] == pk) {
				if(dy1 != 0 && i > 0) {
					Panel *pp = m_kids[i-1];
					float weight_delta = (float)dy1 / pixels_per_weight;
					pk->m_weight -= weight_delta;
					pp->m_weight += weight_delta;
				}
				if(i < (nkids - 1)) {
					Panel *pn = m_kids[i+1];
					float weight_delta = (float)dy2 / pixels_per_weight;
					pk->m_weight += weight_delta;
					pn->m_weight -= weight_delta;
				}
			}
		}
		if (m_parent) {
			m_parent->update_kid(this, dx1, 0, dx2, 0);
		}
	}

}


int Panel::draw(View &view, Streams &streams, SDL_Renderer *rend, int x, int y, int w, int h)
{
	int rv = 0;

	m_last_w = w;
	m_last_h = h;

	if(m_type == Type::Widget) {

		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoCollapse;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoTitleBar;
		flags |= ImGuiWindowFlags_NoSavedSettings;
		flags |= ImGuiWindowFlags_NoNavInputs;

		ImGui::SetNextWindowPos(ImVec2((float)x, (float)y));
		ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));

		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin(m_title, nullptr, flags);

		if(ImGui::IsWindowHovered() && 
		   !ImGui::IsMouseDragging(ImGuiMouseButton_Left) && 
		   !ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			ImGui::SetWindowFocus();
		}
	
		if(ImGui::IsWindowFocused()) {
			if(ImGui::IsKeyPressed(ImGuiKey_V)) {
				if(m_parent->get_type() == Type::SplitV) {
					Panel *pn = new Panel(m_widget->copy());
					m_parent->add(pn, this);
					pn->m_weight *= 0.5f;
					m_weight *= 0.5f;
				} else {
					Panel *pnew = new Panel(Type::SplitV);
					m_parent->replace(this, pnew);
					pnew->add(this);
					pnew->add(new Panel(m_widget->copy()));
				}
			}
			if(ImGui::IsKeyPressed(ImGuiKey_H)) {
				if(m_parent->get_type() == Type::SplitH) {
					Panel *pn = new Panel(m_widget->copy());
					m_parent->add(pn, this);
					pn->m_weight *= 0.5f;
					m_weight *= 0.5f;
				} else {
					Panel *pnew = new Panel(Type::SplitH);
					m_parent->replace(this, pnew);
					pnew->add(this);
					pnew->add(new Panel(m_widget->copy()));
				}
			}
			if(ImGui::IsKeyPressed(ImGuiKey_X)) {
				if(m_parent) {
					Panel *pp = m_parent;
					m_parent->remove(this);
				}
			}
		}

		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();

		if(pos.x != x || pos.y != y || size.x != w || size.y != h) {
			int dx1 = pos.x - x;
			int dy1 = pos.y - y;
			int dx2 = (pos.x + size.x) - (x + w);
			int dy2 = (pos.y + size.y) - (y + h);

			m_parent->update_kid(this, dx1, dy1, dx2, dy2);
		}
		
		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImVec2 avail = ImGui::GetContentRegionAvail();

		SDL_Rect r = {
			(int)cursor.x,
			(int)cursor.y,
			(int)avail.x,
			(int)avail.y
		};

		SDL_SetRenderClipRect(rend, &r);
		m_widget->draw(view, streams, rend, r);
		SDL_SetRenderClipRect(rend, nullptr);

		ImGui::End();

	} else if(m_type == Type::SplitH) {
	
		float total_weight = 0.0f;
		for(auto &pk : m_kids) {
			total_weight += pk->m_weight;
		}

		int kx = x;
		for(auto &pk : m_kids) {
			bool last = (&pk == &m_kids.back());
			int kw = (pk->m_weight / total_weight) * w;
			rv += kw;
			pk->draw(view, streams, rend, kx, y, kw, h);
			kx += kw + 1;
		}

	} else if(m_type == Type::SplitV) {
		
		float total_weight = 0.0f;
		for(auto &pk : m_kids) {
			total_weight += pk->m_weight;
		}

		int ky = y;
		for(auto &pk : m_kids) {
			bool last = (&pk == &m_kids.back());
			int kh = (pk->m_weight / total_weight) * h;
			rv += kh;
			pk->draw(view, streams, rend, x, ky, w, kh);
			ky += kh + 1;
		}

	}

	// remove kids marked for deletion
	for(auto &pk : m_kids_remove) {
		for(size_t i=0; i<m_kids.size(); i++) {
			if(m_kids[i] == pk) {
				m_kids.erase(m_kids.begin() + i);
				pk->m_parent = nullptr;
				delete pk;
			}
		}
	}
	m_kids_remove.clear();

	// add kids marked for addition
	for(auto &req : m_add_requests) {
		if(req.p_after) {
			for(size_t i=0; i<m_kids.size(); i++) {
				if(m_kids[i] == req.p_after) {
					m_kids.insert(m_kids.begin() + i + 1, req.p_new);
					req.p_new->m_parent = this;
				}
			}
		} else {
			m_kids.push_back(req.p_new);
		}
		req.p_new->m_parent = this;
	}
	m_add_requests.clear();


	return rv;
}
