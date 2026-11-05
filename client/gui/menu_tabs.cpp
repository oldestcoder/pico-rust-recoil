#include "../src/game_data.h"
#include "../src/recoil_blatant.h"
#include "../src/recoil_legit.h"
#include "../src/settings_manager.h"
#include "menu_render.h"
#include "menu_styles.hpp"
#include "IconsFontAwesome5.h"
#include <string>

static float gui_sens = 0.28f;
static float gui_adsSens = 0.76f;
static float gui_fov = 90.0f;
static float gui_smoothing = 500.0f;
static float gui_xSpeed = 1.00f;
static float gui_ySpeed = 1.00f;
static bool gui_hipfire = false;
static bool gui_movementDetection = true;
static bool gui_synced = false;

static void SyncFromCore() {
  if (!gui_synced) {
    gui_sens = g_gameState.sensitivity;
    gui_adsSens = g_gameState.adsSensitivity;
    gui_fov = g_gameState.fov;
    gui_smoothing = (float)g_gameState.smoothingSteps;
    gui_xSpeed = g_gameState.xSpeed;
    gui_ySpeed = g_gameState.ySpeed;
    gui_hipfire = g_gameState.hipfireEnabled;
    gui_movementDetection = g_gameState.movementDetectionEnabled;

    static const char *weapons[] = {"AK-47",    "LR300",
                                    "MP5A4",    "Custom SMG",
                                    "Thompson", "HMLMG",
                                    "M249",     "Semi Auto Rifle",
                                    "Revolver", "Semi Automatic Pistol",
                                    "M92",      "M39",
                                    "Python",   "Nail Gun"};
    for (int i = 0; i < 14; i++) {
      if (g_gameState.currentWeapon == weapons[i]) {
        g_dropdownSelections["##dd_Weapon"] = i;
        break;
      }
    }

    static const char *scopeNames[] = {"None", "Holo", "Handmade", "8x"};
    for (int i = 0; i < 4; i++) {
      if (g_gameState.currentScope == scopeNames[i]) {
        g_dropdownSelections["##dd_Scope"] = i;
        break;
      }
    }

    static const char *barrelNames[] = {"None", "Silencer", "Muzzle Boost",
                                        "Muzzle Brake"};
    for (int i = 0; i < 4; i++) {
      if (g_gameState.currentBarrel == barrelNames[i]) {
        g_dropdownSelections["##dd_Barrel"] = i;
        break;
      }
    }

    g_dropdownSelections["##dd_Mode"] = g_gameState.scriptMode;

    g_dropdownSelections["##dd_Select Theme"] = g_gameState.themeIndex;

    gui_synced = true;
  }
}

static bool prev_recoilEnabled = false;
static int prev_scriptMode = -1;
static bool prev_initialized = false;

static void SyncToCore() {
  if (!prev_initialized) {
    prev_recoilEnabled = g_gameState.recoilEnabled;
    prev_scriptMode = g_gameState.scriptMode;
    prev_initialized = true;
  }

  if (g_gameState.recoilEnabled != prev_recoilEnabled) {
    if (g_gameState.recoilEnabled) {
      if (g_gameState.scriptMode == 0)
        g_legitEngine.start();
      else
        g_blatantEngine.start();
    } else {
      g_legitEngine.stop();
      g_blatantEngine.stop();
    }
    prev_recoilEnabled = g_gameState.recoilEnabled;
  }

  if (g_gameState.scriptMode != prev_scriptMode) {
    g_legitEngine.stop();
    g_blatantEngine.stop();
    Sleep(50);
    if (g_gameState.recoilEnabled) {
      if (g_gameState.scriptMode == 0)
        g_legitEngine.start();
      else
        g_blatantEngine.start();
    }
    prev_scriptMode = g_gameState.scriptMode;
  }

  // Avoid rewriting shared settings every render frame when the UI value has
  // not changed. The recoil worker reads these fields concurrently, so needless
  // writes (especially to std::string weapon names below) are both wasteful and
  // increase the chance of observing partially updated state during a change.
  if (g_gameState.sensitivity != gui_sens)
    g_gameState.sensitivity = gui_sens;
  if (g_gameState.adsSensitivity != gui_adsSens)
    g_gameState.adsSensitivity = gui_adsSens;
  if (g_gameState.fov != gui_fov)
    g_gameState.fov = gui_fov;
  if (g_gameState.smoothingSteps != (int)gui_smoothing)
    g_gameState.smoothingSteps = (int)gui_smoothing;
  if (g_gameState.xSpeed != gui_xSpeed)
    g_gameState.xSpeed = gui_xSpeed;
  if (g_gameState.ySpeed != gui_ySpeed)
    g_gameState.ySpeed = gui_ySpeed;
  if (g_gameState.hipfireEnabled != gui_hipfire)
    g_gameState.hipfireEnabled = gui_hipfire;
  if (g_gameState.movementDetectionEnabled != gui_movementDetection)
    g_gameState.movementDetectionEnabled = gui_movementDetection;

  static const char *weapons[] = {"AK-47",    "LR300",
                                  "MP5A4",    "Custom SMG",
                                  "Thompson", "HMLMG",
                                  "M249",     "Semi Auto Rifle",
                                  "Revolver", "Semi Automatic Pistol",
                                  "M92",      "M39",
                                  "Python",   "Nail Gun"};
  auto wIt = g_dropdownSelections.find("##dd_Weapon");
  if (wIt != g_dropdownSelections.end() && wIt->second >= 0 && wIt->second < 14 &&
      g_gameState.currentWeapon != weapons[wIt->second])
    g_gameState.currentWeapon = weapons[wIt->second];

  static const char *scopeNames[] = {"None", "Holo", "Handmade", "8x"};
  auto sIt = g_dropdownSelections.find("##dd_Scope");
  if (sIt != g_dropdownSelections.end() && sIt->second >= 0 && sIt->second < 4 &&
      g_gameState.currentScope != scopeNames[sIt->second])
    g_gameState.currentScope = scopeNames[sIt->second];

  static const char *barrelNames[] = {"None", "Silencer", "Muzzle Boost",
                                      "Muzzle Brake"};
  auto brIt = g_dropdownSelections.find("##dd_Barrel");
  if (brIt != g_dropdownSelections.end() && brIt->second >= 0 &&
      brIt->second < 4 && g_gameState.currentBarrel != barrelNames[brIt->second])
    g_gameState.currentBarrel = barrelNames[brIt->second];

  auto mIt = g_dropdownSelections.find("##dd_Mode");
  if (mIt != g_dropdownSelections.end() && mIt->second >= 0 && mIt->second < 2 &&
      g_gameState.scriptMode != mIt->second)
    g_gameState.scriptMode = mIt->second;
}

void RenderRecoilTab(ImDrawList *dl, ImVec2 pos, float w, float alpha, float dt,
                     bool input, ImVec2 winPos, ImVec2 winSize, int subTab) {
  SyncFromCore();
  float halfW = (w - 30) * 0.5f;
  ImVec2 leftPos = pos;
  ImVec2 rightPos = pos + ImVec2(halfW + 20, 0);
  float screenBot = winPos.y + winSize.y;

  dl->PushClipRect(ImVec2(winPos.x, pos.y),
                   ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), true);
  dl->ChannelsSplit(2);

  float y = leftPos.y;
  float yr = rightPos.y;

  if (subTab == 0) {

    float start_ctrl = y;
    MR_BeginCard(dl, ImVec2(leftPos.x, y), "Recoil Control", halfW, alpha);
    y += 40;
    MR_Checkbox(dl, ImVec2(leftPos.x, y), "Enable Recoil",
                &g_gameState.recoilEnabled, alpha, dt, input);
    y += 30;
    MR_Checkbox(dl, ImVec2(leftPos.x, y), "Hipfire", &gui_hipfire, alpha, dt,
                input);
    y += 30;
    MR_Checkbox(dl, ImVec2(leftPos.x, y), "Movement Detection",
                &gui_movementDetection, alpha, dt, input);
    y += 35;
    y += 5;
    MR_EndCard(dl, ImVec2(leftPos.x, start_ctrl), ImVec2(leftPos.x, y), halfW,
               alpha);
    y += 15;

    MR_Dropdown(dl, ImVec2(leftPos.x, y), halfW - 10, "Mode",
                {"Legit", "Blatant"}, alpha, dt, input, screenBot);
    y += 62;

    MR_Dropdown(dl, ImVec2(rightPos.x, yr), halfW - 10, "Weapon",
                {"AK-47", "LR300", "MP5A4", "Custom SMG", "Thompson", "HMLMG",
                 "M249", "Semi Auto Rifle", "Revolver", "Semi Automatic Pistol",
                 "M92", "M39", "Python", "Nail Gun"},
                alpha, dt, input, screenBot);
    yr += 62;
    MR_Dropdown(dl, ImVec2(rightPos.x, yr), halfW - 10, "Scope",
                {"None", "Holo", "Handmade", "8x"}, alpha, dt, input,
                screenBot);
    yr += 62;
    MR_Dropdown(dl, ImVec2(rightPos.x, yr), halfW - 10, "Barrel",
                {"None", "Silencer", "Muzzle Boost", "Muzzle Brake"}, alpha, dt,
                input, screenBot);
    yr += 62;
  }

  dl->ChannelsMerge();
  dl->PopClipRect();
  SyncToCore();
}

void RenderTuningTab(ImDrawList *dl, ImVec2 pos, float w, float alpha, float dt,
                     bool input, ImVec2 winPos, ImVec2 winSize, int subTab) {
  SyncFromCore();
  float halfW = (w - 30) * 0.5f;
  ImVec2 leftPos = pos;
  ImVec2 rightPos = pos + ImVec2(halfW + 20, 0);

  dl->PushClipRect(ImVec2(winPos.x, pos.y),
                   ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), true);
  dl->ChannelsSplit(2);

  float y = leftPos.y;
  float yr = rightPos.y;

  if (subTab == 0) {
    float start_sens = y;
    MR_BeginCard(dl, ImVec2(leftPos.x, y), "Sensitivity", halfW, alpha);
    y += 40;
    MR_Slider(dl, ImVec2(leftPos.x, y), halfW - 10, "Game Sens", &gui_sens,
              0.01f, 5.0f, alpha, input, "%.2f");
    y += 42;
    MR_Slider(dl, ImVec2(leftPos.x, y), halfW - 10, "ADS Sens", &gui_adsSens,
              0.01f, 5.0f, alpha, input, "%.2f");
    y += 42;
    MR_Slider(dl, ImVec2(leftPos.x, y), halfW - 10, "FOV", &gui_fov, 70.0f,
              90.0f, alpha, input, "%.0f");
    y += 42;
    y += 5;
    MR_EndCard(dl, ImVec2(leftPos.x, start_sens), ImVec2(leftPos.x, y), halfW,
               alpha);

    float start_speed = yr;
    MR_BeginCard(dl, ImVec2(rightPos.x, yr), "Speed Control", halfW, alpha);
    yr += 40;
    MR_Slider(dl, ImVec2(rightPos.x, yr), halfW - 10, "X Speed", &gui_xSpeed,
              0.01f, 2.0f, alpha, input, "%.2f");
    yr += 42;
    MR_Slider(dl, ImVec2(rightPos.x, yr), halfW - 10, "Y Speed", &gui_ySpeed,
              0.01f, 2.0f, alpha, input, "%.2f");
    yr += 42;
    MR_Slider(dl, ImVec2(rightPos.x, yr), halfW - 10, "Smoothing Steps",
              &gui_smoothing, 1.0f, 1000.0f, alpha, input, "%.0f");
    yr += 42;
    yr += 5;
    MR_EndCard(dl, ImVec2(rightPos.x, start_speed), ImVec2(rightPos.x, yr),
               halfW, alpha);
  }

  dl->ChannelsMerge();
  dl->PopClipRect();
  SyncToCore();
}

void RenderSettingsTab(ImDrawList *dl, ImVec2 pos, float w, float alpha,
                       float dt, bool input, ImVec2 winPos, ImVec2 winSize,
                       int subTab) {
  SyncFromCore();
  float halfW = (w - 30) * 0.5f;
  ImVec2 leftPos = pos;
  ImVec2 rightPos = pos + ImVec2(halfW + 20, 0);
  float screenBot = winPos.y + winSize.y;

  dl->PushClipRect(ImVec2(winPos.x, pos.y),
                   ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), true);
  dl->ChannelsSplit(2);

  float y = leftPos.y;
  float yr = rightPos.y;

  if (subTab == 0) {

    float start_kb = y;
    MR_BeginCard(dl, ImVec2(leftPos.x, y), "Keybinds", halfW, alpha);
    y += 40;
    MR_Hotkey(dl, ImVec2(leftPos.x, y), halfW * 0.45f, "Recoil Toggle",
              &g_gameState.toggleKey, alpha, dt, input);
    y += 62;
    MR_Hotkey(dl, ImVec2(leftPos.x, y), halfW * 0.45f, "Hide Menu",
              &g_gameState.hideMenuKey, alpha, dt, input);
    y += 62;
    MR_Hotkey(dl, ImVec2(leftPos.x, y), halfW * 0.45f, "Mode Switch",
              &g_gameState.modeSwitchKey, alpha, dt, input);
    y += 62;
    y += 5;
    MR_EndCard(dl, ImVec2(leftPos.x, start_kb), ImVec2(leftPos.x, y), halfW,
               alpha);

    float start_theme = yr;
    MR_BeginCard(dl, ImVec2(rightPos.x, yr), "Theme", halfW, alpha);
    yr += 40;
    const std::vector<std::string> themes = {"Forge Red", "Deep Ocean",
                                             "Amethyst", "Emerald"};
    if (MR_Dropdown(dl, ImVec2(rightPos.x, yr), halfW - 10, "Select Theme",
                    themes, alpha, dt, input, screenBot)) {
      int idx = g_dropdownSelections["##dd_Select Theme"];
      g_gameState.themeIndex = idx;
      ApplyTheme(idx);
    }
    yr += 62;
    MR_Checkbox(dl, ImVec2(rightPos.x, yr), "Particle Orbs",
                &g_gameState.orbsEnabled, alpha, dt, input);
    yr += 35;
    yr += 5;
    MR_EndCard(dl, ImVec2(rightPos.x, start_theme), ImVec2(rightPos.x, yr),
               halfW, alpha);
  }

  dl->ChannelsMerge();
  dl->PopClipRect();
  SyncToCore();
}

static std::string GenerateBindId() {
  static int counter = 0;
  char buf[64];
  LARGE_INTEGER pc;
  QueryPerformanceCounter(&pc);
  sprintf(buf, "bind_%lld_%d", pc.QuadPart, counter++);
  return buf;
}

static bool MR_Button(ImDrawList *dl, ImVec2 pos, ImVec2 size,
                      const char *label, ImColor bg, ImColor hoverBg,
                      ImColor textColor, float alpha, bool input,
                      const char *id) {
  bool pressed = false;
  dl->AddRectFilled(pos, pos + size,
                    ImColor(bg.Value.x, bg.Value.y, bg.Value.z, alpha), 6.f);
  bool hov = false;
  if (input) {
    ImGui::SetCursorScreenPos(pos);
    if (ImGui::InvisibleButton(id, size))
      pressed = true;
    hov = ImGui::IsItemHovered();
  }
  if (hov)
    dl->AddRectFilled(
        pos, pos + size,
        ImColor(hoverBg.Value.x, hoverBg.Value.y, hoverBg.Value.z, alpha),
        6.f);

  if (g_fontSmall)
    ImGui::PushFont(g_fontSmall);
  ImVec2 ts = ImGui::CalcTextSize(label);
  dl->AddText(pos + ImVec2((size.x - ts.x) * 0.5f, (size.y - ts.y) * 0.5f),
              ImColor(textColor.Value.x, textColor.Value.y, textColor.Value.z,
                      alpha),
              label);
  if (g_fontSmall)
    ImGui::PopFont();
  return pressed;
}

static bool MR_BindDropdown(ImDrawList *dl, ImVec2 pos, float w,
                            const char *ddId,
                            const std::vector<std::string> &opts, float alpha,
                            float dt, bool input, float screenBot) {

  return MR_Dropdown(dl, pos, w, ddId, opts, alpha, dt, input, screenBot);
}

static float g_keybindsScroll = 0.0f;

void RenderKeybindsTab(ImDrawList *dl, ImVec2 pos, float w, float alpha,
                       float dt, bool input, ImVec2 winPos, ImVec2 winSize,
                       int subTab) {
  SyncFromCore();

  static const char *weapons[] = {"AK-47",    "LR300",
                                  "MP5A4",    "Custom SMG",
                                  "Thompson", "HMLMG",
                                  "M249",     "Semi Auto Rifle",
                                  "Revolver", "Semi Automatic Pistol",
                                  "M92",      "M39",
                                  "Python",   "Nail Gun"};
  static const char *scopeNames[] = {"None", "Holo", "Handmade", "8x"};
  static const char *barrelNames[] = {"None", "Silencer", "Muzzle Boost",
                                      "Muzzle Brake"};

  float screenBot = winPos.y + winSize.y;
  float contentH = winSize.y - (pos.y - winPos.y) - 60.f;

  dl->PushClipRect(ImVec2(winPos.x, pos.y),
                   ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), true);
  dl->ChannelsSplit(2);

  float y = pos.y;

  MR_DrawText(dl, g_fontHeader, ImVec2(pos.x, y),
              ImColor(g_accentColor.x, g_accentColor.y, g_accentColor.z, alpha),
              "Custom Keybinds");

  {
    ImVec2 addPos(pos.x + w - 30, y);
    ImVec2 addSize(26, 26);
    if (MR_Button(dl, addPos, addSize, "+", ImColor(30, 30, 30, 200),
                  ImColor(50, 50, 50, 200),
                  ImColor(g_accentColor.x, g_accentColor.y, g_accentColor.z,
                          1.0f),
                  alpha, input, "##add_bind")) {
      CustomKeybind newBind;
      newBind.id = GenerateBindId();
      g_gameState.customKeybinds.push_back(newBind);
      saveSettings();
    }
  }

  y += 35;

  float listTop = y;
  float listH = contentH - 35;
  if (listH < 50)
    listH = 50;

  float totalH = 0;
  for (size_t i = 0; i < g_gameState.customKeybinds.size(); i++) {
    auto &b = g_gameState.customKeybinds[i];
    if (b.actionType == "preset") {
      totalH += b.useCustomSpeed ? 185.f : 120.f;
    } else {
      totalH += 55.f;
    }
    totalH += 10.f;
  }
  if (g_gameState.customKeybinds.empty())
    totalH = 60.f;

  if (input && g_openDropdown.empty()) {
    ImVec2 mpos = ImGui::GetIO().MousePos;
    if (mpos.x >= pos.x && mpos.x <= pos.x + w && mpos.y >= listTop &&
        mpos.y <= listTop + listH) {
      float wheel = ImGui::GetIO().MouseWheel;
      if (wheel != 0.f)
        g_keybindsScroll -= wheel * 40.f;
    }
  }
  float maxScroll = totalH - listH;
  if (maxScroll < 0)
    maxScroll = 0;
  if (g_keybindsScroll < 0)
    g_keybindsScroll = 0;
  if (g_keybindsScroll > maxScroll)
    g_keybindsScroll = maxScroll;

  dl->PushClipRect(ImVec2(winPos.x, listTop),
                   ImVec2(winPos.x + winSize.x, listTop + listH), true);

  float cy = listTop - g_keybindsScroll;
  int deleteIdx = -1;

  if (g_gameState.customKeybinds.empty()) {

    const char *emptyMsg = "Click + to add a keybind or preset";
    if (g_fontSmall)
      ImGui::PushFont(g_fontSmall);
    ImVec2 ts = ImGui::CalcTextSize(emptyMsg);
    if (g_fontSmall)
      ImGui::PopFont();
    MR_DrawText(dl, g_fontSmall,
                ImVec2(pos.x + (w - ts.x) * 0.5f, cy + 15),
                ImColor(100, 100, 100, (int)(255 * alpha)), emptyMsg);
  }

  for (size_t i = 0; i < g_gameState.customKeybinds.size(); i++) {
    auto &b = g_gameState.customKeybinds[i];
    bool isPreset = (b.actionType == "preset");
    float stripH = isPreset ? (b.useCustomSpeed ? 180.f : 115.f) : 50.f;

    if (cy + stripH < listTop - 5 || cy > listTop + listH + 5) {
      cy += stripH + 10.f;
      continue;
    }

    ImVec2 sp(pos.x, cy);
    dl->AddRectFilled(sp, sp + ImVec2(w, stripH),
                      ImColor(255, 255, 255, (int)(8 * alpha)), 8.f);
    dl->AddRect(sp, sp + ImVec2(w, stripH),
                ImColor(30, 30, 30, (int)(255 * alpha)), 8.f);

    char idBuf[64];

    if (!isPreset) {

      float pad = 10.f;
      float usable = w - pad * 2;

      float actW = 80.f;
      float valW = 110.f;
      float kbW  = 50.f;
      float thW  = 50.f;
      float enW  = 20.f;
      float delW = 22.f;
      float totalElems = actW + valW + kbW + thW + enW + delW;
      float gap = (usable - totalElems) / 5.f;
      if (gap < 4.f) gap = 4.f;

      float cx = sp.x + pad;
      float syy = sp.y + 11;

      sprintf(idBuf, "##bind_act_%d", (int)i);
      {

        std::string ddActKey = "##dd_" + std::string(idBuf);
        if (g_dropdownSelections.find(ddActKey) == g_dropdownSelections.end()) {
          int actIdx = 0;
          if (b.actionType == "weapon")
            actIdx = 1;
          else if (b.actionType == "attachment")
            actIdx = 2;
          else if (b.actionType == "barrel")
            actIdx = 3;
          else if (b.actionType == "preset")
            actIdx = 4;
          g_dropdownSelections[ddActKey] = actIdx;
        }
      }
      if (MR_Dropdown(dl, ImVec2(cx, syy), actW, idBuf,
                      {"Option", "Weapon", "Scope", "Barrel", "Preset"}, alpha,
                      dt, input, screenBot)) {
        std::string ddActKey = "##dd_" + std::string(idBuf);
        int sel = g_dropdownSelections[ddActKey];
        const char *types[] = {"option", "weapon", "attachment", "barrel",
                               "preset"};
        std::string newType = types[sel];
        if (newType != b.actionType) {
          b.actionType = newType;

          char valId[64];
          sprintf(valId, "##bind_val_%d", (int)i);
          std::string ddValKey = "##dd_" + std::string(valId);
          g_dropdownSelections.erase(ddValKey);

          if (newType == "option")
            b.actionValue = "Master Toggle";
          else if (newType == "weapon")
            b.actionValue = "AK-47";
          else if (newType == "attachment")
            b.actionValue = "None";
          else if (newType == "barrel")
            b.actionValue = "None";
          else if (newType == "preset") {
            b.presetWeapon = "AK-47";
            b.presetScope = "None";
            b.presetBarrel = "None";
          }
        }
        saveSettings();
      }
      cx += actW + gap;

      sprintf(idBuf, "##bind_val_%d", (int)i);
      std::vector<std::string> valOpts;
      std::string curActionType = b.actionType;
      if (curActionType == "option")
        valOpts = {"Master Toggle", "Hide Menu", "Hipfire Toggle"};
      else if (curActionType == "weapon")
        for (auto wn : weapons)
          valOpts.push_back(wn);
      else if (curActionType == "attachment")
        for (auto sn : scopeNames)
          valOpts.push_back(sn);
      else if (curActionType == "barrel")
        for (auto bn : barrelNames)
          valOpts.push_back(bn);

      if (!valOpts.empty()) {
        std::string ddValKey = "##dd_" + std::string(idBuf);
        if (g_dropdownSelections.find(ddValKey) == g_dropdownSelections.end()) {
          int vi = 0;
          for (int v = 0; v < (int)valOpts.size(); v++) {
            if (valOpts[v] == b.actionValue) {
              vi = v;
              break;
            }
          }
          g_dropdownSelections[ddValKey] = vi;
        }
        if (MR_Dropdown(dl, ImVec2(cx, syy), valW, idBuf, valOpts,
                        alpha, dt, input, screenBot)) {
          int sel = g_dropdownSelections[ddValKey];
          if (sel >= 0 && sel < (int)valOpts.size())
            b.actionValue = valOpts[sel];
          saveSettings();
        }
      }
      cx += valW + gap;

      sprintf(idBuf, "bind_key_%d", (int)i);
      MR_HotkeyInline(dl, ImVec2(cx, sp.y + 12), kbW, idBuf,
                      &b.keybind, alpha, dt, input);
      cx += kbW + gap;

      bool isToggle = (b.type == "toggle");
      sprintf(idBuf, "##bind_th_%d", (int)i);
      const char *thLabel = isToggle ? "Toggle" : "Hold";
      ImColor thBg =
          isToggle ? ImColor(16, 185, 129, (int)(50 * alpha))
                   : ImColor(245, 158, 11, (int)(50 * alpha));
      ImColor thBorder =
          isToggle
              ? ImColor(g_accentColor.x, g_accentColor.y, g_accentColor.z,
                        alpha)
              : ImColor(0.96f, 0.62f, 0.04f, alpha);

      ImVec2 thPos(cx, sp.y + 12);
      ImVec2 thSize(thW, 26);
      dl->AddRectFilled(thPos, thPos + thSize, thBg, 6.f);
      dl->AddRect(thPos, thPos + thSize, thBorder, 6.f);
      if (g_fontSmall)
        ImGui::PushFont(g_fontSmall);
      ImVec2 thTs = ImGui::CalcTextSize(thLabel);
      dl->AddText(
          thPos + ImVec2((thSize.x - thTs.x) * 0.5f, (thSize.y - thTs.y) * 0.5f),
          thBorder, thLabel);
      if (g_fontSmall)
        ImGui::PopFont();
      if (input) {
        ImGui::SetCursorScreenPos(thPos);
        if (ImGui::InvisibleButton(idBuf, thSize)) {
          b.type = isToggle ? "hold" : "toggle";
          saveSettings();
        }
      }
      cx += thW + gap;

      sprintf(idBuf, "##bind_en_%d", (int)i);
      MR_Checkbox(dl, ImVec2(cx, sp.y + 15), idBuf, &b.enabled, alpha, dt,
                  input);
      cx += enW + gap;

      sprintf(idBuf, "##bind_del_%d", (int)i);
      if (MR_Button(dl, ImVec2(cx, sp.y + 13), ImVec2(delW, 24), "X",
                    ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0),
                    ImColor(100, 100, 100, 255), alpha, input, idBuf)) {
        deleteIdx = (int)i;
      }

    } else {

      float px = sp.x + 15;
      float py = sp.y + 12;

      char titleBuf[32];
      sprintf(titleBuf, "Preset %d", (int)(i + 1));
      MR_DrawText(dl, g_fontSmall, ImVec2(px, py),
                  ImColor(255, 255, 255, (int)(255 * alpha)), titleBuf);

      float delX = sp.x + w - 30;
      sprintf(idBuf, "##bind_del_%d", (int)i);
      if (MR_Button(dl, ImVec2(delX, py), ImVec2(20, 20), "X",
                    ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0),
                    ImColor(100, 100, 100, 255), alpha, input, idBuf)) {
        deleteIdx = (int)i;
      }
      py += 30;

      float ddW = (w - 50) * 0.4f;
      float dd2W = (w - 50) * 0.3f;

      sprintf(idBuf, "##preset_wpn_%d", (int)i);
      {
        std::vector<std::string> wOpts;
        for (auto wn : weapons)
          wOpts.push_back(wn);
        std::string ddKey = "##dd_" + std::string(idBuf);
        if (g_dropdownSelections.find(ddKey) == g_dropdownSelections.end()) {
          for (int wi = 0; wi < (int)wOpts.size(); wi++) {
            if (wOpts[wi] == b.presetWeapon) {
              g_dropdownSelections[ddKey] = wi;
              break;
            }
          }
        }
        if (MR_Dropdown(dl, ImVec2(px, py), ddW, idBuf, wOpts, alpha, dt,
                        input, screenBot)) {
          int sel = g_dropdownSelections[ddKey];
          if (sel >= 0 && sel < (int)wOpts.size())
            b.presetWeapon = wOpts[sel];
          saveSettings();
        }
      }

      sprintf(idBuf, "##preset_scp_%d", (int)i);
      {
        std::vector<std::string> sOpts;
        for (auto sn : scopeNames)
          sOpts.push_back(sn);
        std::string ddKey = "##dd_" + std::string(idBuf);
        if (g_dropdownSelections.find(ddKey) == g_dropdownSelections.end()) {
          for (int si = 0; si < (int)sOpts.size(); si++) {
            if (sOpts[si] == b.presetScope) {
              g_dropdownSelections[ddKey] = si;
              break;
            }
          }
        }
        if (MR_Dropdown(dl, ImVec2(px + ddW + 10, py), dd2W, idBuf, sOpts,
                        alpha, dt, input, screenBot)) {
          int sel = g_dropdownSelections[ddKey];
          if (sel >= 0 && sel < (int)sOpts.size())
            b.presetScope = sOpts[sel];
          saveSettings();
        }
      }

      sprintf(idBuf, "##preset_brl_%d", (int)i);
      {
        std::vector<std::string> brOpts;
        for (auto bn : barrelNames)
          brOpts.push_back(bn);
        std::string ddKey = "##dd_" + std::string(idBuf);
        if (g_dropdownSelections.find(ddKey) == g_dropdownSelections.end()) {
          for (int bi = 0; bi < (int)brOpts.size(); bi++) {
            if (brOpts[bi] == b.presetBarrel) {
              g_dropdownSelections[ddKey] = bi;
              break;
            }
          }
        }
        if (MR_Dropdown(dl, ImVec2(px + ddW + dd2W + 20, py), dd2W, idBuf,
                        brOpts, alpha, dt, input, screenBot)) {
          int sel = g_dropdownSelections[ddKey];
          if (sel >= 0 && sel < (int)brOpts.size())
            b.presetBarrel = brOpts[sel];
          saveSettings();
        }
      }
      py += 35;

      MR_DrawText(dl, g_fontSmall, ImVec2(px, py + 3),
                  ImColor(100, 100, 100, (int)(255 * alpha)), "Keybind:");
      sprintf(idBuf, "preset_key_%d", (int)i);
      MR_HotkeyInline(dl, ImVec2(px + 65, py), 60, idBuf, &b.keybind, alpha, dt,
                input);

      float offX = sp.x + w - 150;
      MR_DrawText(dl, g_fontSmall, ImVec2(offX, py + 3),
                  ImColor(100, 100, 100, (int)(255 * alpha)), "Custom Offset:");
      sprintf(idBuf, "##preset_off_%d", (int)i);
      MR_Checkbox(dl, ImVec2(offX + 105, py + 1), idBuf, &b.useCustomSpeed,
                  alpha, dt, input);

      if (b.useCustomSpeed) {
        py += 35;

        float slW = (w - 60) * 0.5f - 10;
        sprintf(idBuf, "X Speed##ps_x_%d", (int)i);
        MR_Slider(dl, ImVec2(px, py), slW, idBuf, &b.presetXSpeed, 0.1f, 5.0f,
                  alpha, input, "%.1f");

        sprintf(idBuf, "Y Speed##ps_y_%d", (int)i);
        MR_Slider(dl, ImVec2(px + slW + 20, py), slW, idBuf, &b.presetYSpeed,
                  0.1f, 5.0f, alpha, input, "%.1f");
      }
    }

    cy += stripH + 10.f;
  }

  if (deleteIdx >= 0 && deleteIdx < (int)g_gameState.customKeybinds.size()) {

    char cleanBuf[64];
    sprintf(cleanBuf, "##bind_act_%d", deleteIdx);
    g_dropdownSelections.erase(cleanBuf);
    sprintf(cleanBuf, "##bind_val_%d", deleteIdx);
    g_dropdownSelections.erase(cleanBuf);

    g_gameState.customKeybinds.erase(g_gameState.customKeybinds.begin() +
                                     deleteIdx);

    for (int ri = deleteIdx; ri < (int)g_gameState.customKeybinds.size(); ri++) {
      char oldKey[64], newKey[64];
      sprintf(oldKey, "##bind_act_%d", ri + 1);
      sprintf(newKey, "##bind_act_%d", ri);
      if (g_dropdownSelections.count(oldKey)) {
        g_dropdownSelections[newKey] = g_dropdownSelections[oldKey];
        g_dropdownSelections.erase(oldKey);
      }
      sprintf(oldKey, "##bind_val_%d", ri + 1);
      sprintf(newKey, "##bind_val_%d", ri);
      if (g_dropdownSelections.count(oldKey)) {
        g_dropdownSelections[newKey] = g_dropdownSelections[oldKey];
        g_dropdownSelections.erase(oldKey);
      }
    }
    saveSettings();
  }

  dl->PopClipRect();

  float footerY = listTop + listH + 5;
  dl->AddLine(ImVec2(pos.x, footerY), ImVec2(pos.x + w, footerY),
              ImColor(255, 255, 255, (int)(13 * alpha)));
  footerY += 8;

  struct DefaultBind {
    const char *label;
    int *key;
  };
  DefaultBind defaults[] = {
      {"Crouch", &g_gameState.crouchKey},
      {"Left", &g_gameState.moveLeftKey},
      {"Down", &g_gameState.moveDownKey},
      {"Right", &g_gameState.moveRightKey},
      {"Up", &g_gameState.moveUpKey},
  };

  float fx = pos.x + 5;
  for (int di = 0; di < 5; di++) {
    MR_DrawText(dl, g_fontTiny, ImVec2(fx, footerY + 5),
                ImColor(100, 100, 100, (int)(255 * alpha)), defaults[di].label);

    if (g_fontTiny) ImGui::PushFont(g_fontTiny);
    float lblW = ImGui::CalcTextSize(defaults[di].label).x;
    if (g_fontTiny) ImGui::PopFont();
    fx += lblW + 6;

    char hkId[32];
    sprintf(hkId, "def_key_%d", di);
    MR_HotkeyInline(dl, ImVec2(fx, footerY), 45, hkId, defaults[di].key, alpha,
              dt, input);
    fx += 52;

    if (di < 4) {
      MR_DrawText(dl, g_fontTiny, ImVec2(fx, footerY + 3),
                  ImColor(255, 255, 255, (int)(38 * alpha)), "|");
      fx += 12;
    }
  }

  dl->ChannelsMerge();
  dl->PopClipRect();
  SyncToCore();
}
