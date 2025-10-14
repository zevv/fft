
#include <stdlib.h>
#include <imgui.h>

#include "panel.hpp"
	

Panel::Panel(Widget *widget)
	: m_parent(nullptr)
	, m_widget(widget)
	, m_type(Type::Widget)
	, m_size(100)
{
	char buf[16] = "";
	for(int i=0; i<15; i++) {
		char c = 'a' + (rand() % 26);
		buf[i] = c;
	}
	buf[15] = 0;
	m_title = strdup(buf);
}


Panel::Panel(const char *title, size_t size, DrawFn fn)
	: m_parent(nullptr)
	, m_title(title)
	, m_type(Type::Container)
	, m_size(size)
	, m_fn_draw(fn)
{
}


Panel::Panel(Type type)
	: m_parent(nullptr)
	, m_type(type)
	, m_size(100)
	, m_fn_draw(nullptr)
	, m_kids{}
{
}


void Panel::add(Panel *p)
{
	m_kids.push_back(p);
	p->m_parent = this;
}


void Panel::add(Widget *w)
{
	Panel *p = new Panel(w);
	add(p);
}


void Panel::update_kid(Panel *pk, int dx, int dy, int dw, int dh)
{
	if(m_type == Type::SplitH) {
		for(size_t i=0; i<m_kids.size(); i++) {
			if(m_kids[i] == pk) {
				pk->m_size += dw;
				if(i > 0) {
					Panel *pp = m_kids[i-1];
					pp->m_size -= dw;
				}
			}
		}
	}
	if(m_type == Type::SplitV) {
		for(size_t i=0; i<m_kids.size(); i++) {
			if(m_kids[i] == pk) {
				pk->m_size += dh;
				if(i > 0) {
					Panel *pp = m_kids[i-1];
					pp->m_size -= dh;
				}
			}
		}
	}
	if(pk->m_parent) {
		pk->m_parent->update_kid(this, dx, dy, dw, dh);
	}
}


int Panel::draw(SDL_Renderer *rend, int x, int y, int w, int h)
{
	
	if(m_type == Type::Widget) {

		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoCollapse;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoTitleBar;
		flags |= ImGuiWindowFlags_NoSavedSettings;

		//ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::SetNextWindowPos(ImVec2((float)x, (float)y));
		ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));

		ImGui::Begin(m_title, nullptr, flags);

		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();

		if(pos.x != x || pos.y != y || size.x != w || size.y != h) {
			m_parent->update_kid(this, pos.x - x, pos.y - y, size.x - w, size.y - h);
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
		m_widget->draw(rend, r);
		SDL_SetRenderClipRect(rend, nullptr);

		ImGui::End();
	}
	
	else if(m_type == Type::Container) {

		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoCollapse;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoTitleBar;
		flags |= ImGuiWindowFlags_NoSavedSettings;

		//ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::SetNextWindowPos(ImVec2((float)x, (float)y));
		ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
		ImGui::Begin(m_title, nullptr, flags);


		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();

		if(pos.x != x || pos.y != y || size.x != w || size.y != h) {
			m_parent->update_kid(this, pos.x - x, pos.y - y, size.x - w, size.y - h);
		}
		
		if(m_fn_draw) {
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 avail = ImGui::GetContentRegionAvail();

			SDL_Rect r = {
				(int)cursor.x,
				(int)cursor.y,
				(int)avail.x,
				(int)avail.y
			};

			SDL_SetRenderClipRect(rend, &r);
			m_fn_draw(r);
			SDL_SetRenderClipRect(rend, nullptr);
		}
		ImGui::End();

	} else if(m_type == Type::SplitH) {

		int kx = x;
		for(auto &pk : m_kids) {
			bool last = (&pk == &m_kids.back());
			int kw = last ? (w - (kx - x)) : pk->m_size;
			pk->draw(rend, kx, y, kw, h);
			kx += kw + 1;
		}


	} else if(m_type == Type::SplitV) {

		int ky = y;
		for(auto &pk : m_kids) {
			bool last = (&pk == &m_kids.back());
			int kh = last ? (h - (ky - y)) : pk->m_size;
			pk->draw(rend, x, ky, w, kh);
			ky += kh + 1;
		}

	}

	return m_size;
}

