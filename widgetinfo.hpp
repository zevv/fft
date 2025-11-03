
#pragma once

#include <vector>
#include <generator>

#include <imgui.h>

class Widgets;
class Widget;

struct WidgetInfo {

	enum Flags {
		ShowChannelMap	   = 1 << 0,
		ShowLock	       = 1 << 1,
		ShowWindowSize     = 1 << 2,
		ShowWindowType     = 1 << 3,
	};

	const char *name;
	const char *description;
	enum ImGuiKey hotkey;
	int flags;
	Widget *(*fn_create)();
};


class Widgets {
public:
	Widgets(WidgetInfo reg);
	static Widget *draw(const char *name);
	static Widget *create_widget(const char *name);
};


#define REGISTER_WIDGET(class, ...) \
	static WidgetInfo reg = { \
		__VA_ARGS__ \
		.fn_create = []() -> Widget* { return new class(reg); }, \
	}; \
	static Widgets info(reg);


