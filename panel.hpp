
#include <functional>
#include <vector>

#include <SDL3/SDL.h>

#include "widget.hpp"


class Panel {

public:

	typedef std::function<void(SDL_Rect &r)> DrawFn;

	enum class Type {
		Widget, Container, SplitH, SplitV
	};

	Panel(Widget *widget);
	Panel(const char *title, size_t size, DrawFn fn);
	Panel(Type type);
	void add(Panel *p);
	void add(Widget *widget);
	void update_kid(Panel *pk, int dx, int dy, int dw, int dh);
	int draw(SDL_Renderer *rend, int x, int y, int w, int h);

private:

	Panel *m_parent;
	Widget *m_widget;
	const char *m_title;
	Type m_type;
	int m_size;
	DrawFn m_fn_draw;
	std::vector<Panel *> m_kids;
};

