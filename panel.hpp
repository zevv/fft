
#include <functional>
#include <vector>

#include <SDL3/SDL.h>

#include "widget.hpp"
#include "stream.hpp"
#include "view.hpp"
#include "config.hpp"

class Panel {

public:

	enum class Type {
		None, Widget, SplitH, SplitV
	};

	Panel(Widget *widget, int size);
	Panel(Type type, int size = 100);

	void save(ConfigWriter &cfg);
	void load(ConfigReader::Node *node);

	void add(Panel *p);
	void add(Widget *widget, int size);
	void update_kid(Panel *pk, int dx, int dy, int dw, int dh);
	int draw(View &view, Streams &streams, SDL_Renderer *rend, int x, int y, int w, int h);

	Type get_type() { return m_type; }

private:

	Panel *m_parent;
	Widget *m_widget;
	const char *m_title;
	Type m_type;
	int m_size;
	std::vector<Panel *> m_kids;
};

