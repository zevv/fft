
#include <functional>
#include <vector>

#include <SDL3/SDL.h>


class Panel {

public:

	typedef std::function<void(SDL_Rect &r)> DrawFn;

	enum class Type {
		Container, SplitH, SplitV
	};

	Panel(const char *title, size_t size, DrawFn fn);
	Panel(Type type);
	void add(Panel *p);
	void update_kid(Panel *pk, int dx, int dy, int dw, int dh);
	int draw(SDL_Renderer *rend, int x, int y, int w, int h);

private:

	const char *m_title;
	Type m_type;
	int m_size;
	DrawFn m_fn_draw;
	std::vector<Panel *> m_kids;
	Panel *m_parent;
};

