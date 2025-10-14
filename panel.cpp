
#include <stdlib.h>
#include <imgui.h>

#include "panel.hpp"
	

Panel::Panel(Widget *widget, int size)
	: m_parent(nullptr)
	, m_widget(widget)
	, m_type(Type::Widget)
	, m_size(size)
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
	, m_type(type)
	, m_size(100)
	, m_kids{}
{
}


void Panel::add(Panel *p)
{
	m_kids.push_back(p);
	p->m_parent = this;
}


void Panel::add(Widget *w, int size)
{
	Panel *p = new Panel(w, size);
	add(p);
}


void Panel::update_kid(Panel *pk, int dx1, int dy1, int dx2, int dy2)
{
	size_t nkids = m_kids.size();
	if(m_type == Type::SplitH) {
		for(size_t i=0; i<m_kids.size(); i++) {
			if(m_kids[i] == pk) {
				if(dx1 != 0 && i > 0) {
					pk->m_size -= dx1;
					Panel *pp = m_kids[i-1];
					pp->m_size += dx1;
				}
				if(i < (nkids - 1)) {
					pk->m_size += dx2;
					Panel *pn = m_kids[i+1];
					pn->m_size -= dx2;
				}
			}
		}
	}
	if(m_type == Type::SplitV) {
		for(size_t i=0; i<m_kids.size(); i++) {
			if(m_kids[i] == pk) {
				if(dy1 != 0 && i > 0) {
					pk->m_size -= dy1;
					Panel *pp = m_kids[i-1];
					pp->m_size += dy1;
				}
				if(i < (nkids - 1)) {
					pk->m_size += dy2;
					Panel *pn = m_kids[i+1];
					pn->m_size -= dy2;
				}
			}
		}
	}
	if(pk->m_parent) {
		pk->m_parent->update_kid(this, dx1, dy1, dx2, dy2);
	}
}


int Panel::draw(Streams &streams, SDL_Renderer *rend, int x, int y, int w, int h)
{
	
	if(m_type == Type::Widget) {

		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoCollapse;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoTitleBar;
		flags |= ImGuiWindowFlags_NoSavedSettings;

		ImGui::SetNextWindowPos(ImVec2((float)x, (float)y));
		ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));

		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin(m_title, nullptr, flags);

		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();

		if(pos.x != x || pos.y != y || size.x != w || size.y != h) {
			m_parent->update_kid(this, pos.x - x, pos.y - y, 
					size.x - w - x + pos.x, size.y - h - y + pos.y);
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
		m_widget->draw(streams, rend, r);
		SDL_SetRenderClipRect(rend, nullptr);

		ImGui::End();

	} else if(m_type == Type::SplitH) {

		int kx = x;
		for(auto &pk : m_kids) {
			bool last = (&pk == &m_kids.back());
			int kw = last ? (w - (kx - x)) : pk->m_size;
			pk->draw(streams, rend, kx, y, kw, h);
			kx += kw + 1;
		}


	} else if(m_type == Type::SplitV) {

		int ky = y;
		for(auto &pk : m_kids) {
			bool last = (&pk == &m_kids.back());
			int kh = last ? (h - (ky - y)) : pk->m_size;
			pk->draw(streams, rend, x, ky, w, kh);
			ky += kh + 1;
		}

	}

	return m_size;
}
