#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_internal.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

extern int g_activeTab;
extern int g_prevTab;
extern float g_tabAnimProgress;
extern float g_tabIndicatorX;
extern float g_tabIndicatorW;
extern float g_textAnimProgress;

extern bool g_menuVisible;
extern bool g_running;

extern ImVec4 g_accentColor;
extern ImVec4 g_textCol;
extern ImVec4 g_textDisabled;

extern ImFont *g_menuFont;
extern ImFont *g_menuFontSmall;
extern ImFont *g_menuFontLarge;
extern ImFont *g_fontSmall;
extern ImFont *g_fontHeader;
extern ImFont *g_fontTiny;
extern ImFont *g_fontDisplay;

extern std::string g_openDropdown;
extern std::map<std::string, float> g_dropdownAnims;
extern std::map<std::string, int> g_dropdownSelections;
extern std::map<std::string, float> g_checkboxAnims;
extern std::map<std::string, float> g_sliderAnims;
extern bool g_waitingForKey;
extern int *g_keyTarget;
extern std::string g_keyId;

extern bool g_showColorPicker;
extern float g_colorPickerAnim;
extern ImVec4 *g_colorEditTarget;

void ApplyTheme(int themeIndex);
bool InitMainMenu();
void RenderMainMenu();
void CleanupMainMenu();
