
#pragma once

#include <map>

#include "misc.hpp"


struct HotkeyEntry {
	int key;
	const char *action;
};


class Hotkey {

public:
	static bool down(int key, const char *action);
	static bool pressed(int key, const char *action);
	static std::map<int, HotkeyEntry> &hotkeys();
	static void clear();
	static void dump();

};

