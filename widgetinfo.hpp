
#pragma once

#include <vector>
#include <generator>

#include <imgui.h>

class Widgets;
class Widget;

struct WidgetInfo {
	const char *name;
	const char *description;
	enum ImGuiKey hotkey;
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


