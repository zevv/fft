
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

	Panel(Widget *widget);
	Panel(Type type);
	~Panel();

	void save(ConfigWriter &cfg);
	void load(ConfigReader::Node *node);

	void add(Panel *p, Panel *p_after = nullptr);
	void add(Widget *widget);
	void replace(Panel *kid_old, Panel *kid_new);
	void remove(Panel *kid);
	void update_kid(Panel *pk, int dx, int dy, int dw, int dh);
	int draw(View &view, Streams &streams, SDL_Renderer *rend, int x, int y, int w, int h);

	Type get_type() { return m_type; }

private:

	struct AddRequest {
		Panel *p_new;
		Panel *p_after;
	};

	Panel *m_parent;
	Widget *m_widget;
	int m_last_w;
	int m_last_h;
	const char *m_title;
	Type m_type;
	float m_weight;
	std::vector<Panel *> m_kids;
	std::vector<Panel *> m_kids_remove;
	std::vector<AddRequest> m_add_requests;
};

