#include "menu_globals.h"
#include "../src/game_data.h"
#include "menu_styles.hpp"

int g_activeTab = 0;
int g_prevTab = 0;
float g_tabAnimProgress = 1.0f;
float g_tabIndicatorX = 0.0f;
float g_tabIndicatorW = 0.0f;
float g_textAnimProgress = 1.0f;

bool g_menuVisible = true;
bool g_running = true;

ImVec4 g_accentColor = ImVec4(0.0f, 0.6f, 1.0f, 1.0f);
ImVec4 g_textCol = ImVec4(0.9f, 0.9f, 0.95f, 1.0f);
ImVec4 g_textDisabled = ImVec4(0.5f, 0.5f, 0.55f, 1.0f);

ImFont *g_menuFont = nullptr;
ImFont *g_menuFontSmall = nullptr;
ImFont *g_menuFontLarge = nullptr;
ImFont *g_fontSmall = nullptr;
ImFont *g_fontHeader = nullptr;
ImFont *g_fontTiny = nullptr;
ImFont *g_fontDisplay = nullptr;

std::string g_openDropdown = "";
std::map<std::string, float> g_dropdownAnims;
std::map<std::string, int> g_dropdownSelections;
std::map<std::string, float> g_checkboxAnims;
std::map<std::string, float> g_sliderAnims;
bool g_waitingForKey = false;
int *g_keyTarget = nullptr;
std::string g_keyId = "";
bool g_showColorPicker = false;
float g_colorPickerAnim = 0.0f;
ImVec4 *g_colorEditTarget = nullptr;

void ApplyTheme(int themeIndex) {
  if (themeIndex == 0)
    g_accentColor = ImVec4(1.0f, 0.08f, 0.08f, 1.0f);
  else if (themeIndex == 1)
    g_accentColor = ImVec4(0.0f, 0.6f, 1.0f, 1.0f);
  else if (themeIndex == 2)
    g_accentColor = ImVec4(0.6f, 0.2f, 1.0f, 1.0f);
  else if (themeIndex == 3)
    g_accentColor = ImVec4(0.0f, 0.8f, 0.4f, 1.0f);
  else {
    g_accentColor = ImVec4(0.0f, 0.6f, 1.0f, 1.0f);
    g_gameState.themeIndex = 1;
  }

  menu_colors::accent = g_accentColor;
  auto clampF = [](float a, float b) { return a < b ? a : b; };
  menu_colors::accent_bright =
      ImVec4(clampF(g_accentColor.x * 1.5f, 1.0f),
             clampF(g_accentColor.y * 1.5f, 1.0f),
             clampF(g_accentColor.z * 1.5f, 1.0f), 1.0f);
  menu_colors::accent_dim =
      ImVec4(g_accentColor.x * 0.35f, g_accentColor.y * 0.35f,
             g_accentColor.z * 0.35f, 1.0f);
  menu_colors::button_hover =
      ImVec4(0.12f + g_accentColor.x * 0.1f, 0.12f + g_accentColor.y * 0.1f,
             0.12f + g_accentColor.z * 0.1f, 1.0f);
  menu_colors::button_active =
      ImVec4(0.18f + g_accentColor.x * 0.2f, 0.18f + g_accentColor.y * 0.2f,
             0.18f + g_accentColor.z * 0.2f, 1.0f);
}
