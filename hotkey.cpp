
#include <stdio.h>

#include <imgui.h>

#include "hotkey.hpp"


static std::map<int, HotkeyEntry> m_hotkeys;


bool Hotkey::down(int key, const char *action)
{
	m_hotkeys[key] = { key, action };
	return ImGui::IsKeyChordDown((ImGuiKey)key);
}


bool Hotkey::pressed(int key, const char *action)
{
	m_hotkeys[key] = { key, action };
	return ImGui::IsKeyChordPressed((ImGuiKey)key);
}


std::map<int, HotkeyEntry>& Hotkey::hotkeys()
{
	return m_hotkeys;
}


void Hotkey::clear()
{
	m_hotkeys.clear();
}

void Hotkey::dump()
{
	printf("Hotkeys:\n");
	for (const auto& [chord, entry] : m_hotkeys) {
		int key = chord & ~0xf000;
		printf("  %s: %s\n", ImGui::GetKeyName((ImGuiKey)key), entry.action);
	}
}

