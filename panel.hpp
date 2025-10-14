
#include <functional>
#include <vector>

#include <SDL3/SDL.h>

#include "widget.hpp"
#include "stream.hpp"


class Panel {

public:

	enum class Type {
		Widget, SplitH, SplitV
	};

	Panel(Widget *widget);
	Panel(Type type);
	void add(Panel *p);
	void add(Widget *widget);
	void update_kid(Panel *pk, int dx, int dy, int dw, int dh);
	int draw(Streams &streams, SDL_Renderer *rend, int x, int y, int w, int h);

private:

	Panel *m_parent;
	Widget *m_widget;
	const char *m_title;
	Type m_type;
	int m_size;
	std::vector<Panel *> m_kids;
};

