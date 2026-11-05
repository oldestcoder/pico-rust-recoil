#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu_globals.h"
#include "menu_styles.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <windows.h>

inline float MR_Damp(float s, float t, float sm, float dt) {
  return s + (t - s) * (1.0f - powf(2.0f, -sm * dt));
}
inline ImVec2 MR_DampVec(ImVec2 s, ImVec2 t, float sm, float dt) {
  return ImVec2(MR_Damp(s.x, t.x, sm, dt), MR_Damp(s.y, t.y, sm, dt));
}
inline float MR_Lerp(float a, float b, float t) { return a + (b - a) * t; }

inline void MR_DrawText(ImDrawList *dl, ImFont *font, ImVec2 pos, ImColor col,
                        const char *text) {
  if (font)
    ImGui::PushFont(font);
  int len = (int)strlen(text);
  int vis = (int)(len * g_textAnimProgress);
  if (vis > len)
    vis = len;
  std::string s = std::string(text).substr(0, vis);
  if (g_textAnimProgress < 1.0f && fmodf((float)ImGui::GetTime(), 0.5f) > 0.25f)
    s += "_";
  dl->AddText(pos, col, s.c_str());
  if (font)
    ImGui::PopFont();
}

inline void MR_SectionHeader(ImDrawList *dl, ImVec2 pos, const char *text,
                             float a) {
  MR_DrawText(dl, g_fontHeader, pos + ImVec2(5, 0),
              ImColor(g_accentColor.x, g_accentColor.y, g_accentColor.z, a),
              text);
  dl->AddRectFilled(
      pos + ImVec2(5, 25), pos + ImVec2(300, 26),
      ImColor(g_accentColor.x, g_accentColor.y, g_accentColor.z, a * 0.4f));
}

inline void MR_BeginCard(ImDrawList *dl, ImVec2 pos, const char *title, float w,
                         float alpha) {

  MR_DrawText(dl, g_fontSmall, pos + ImVec2(5, 5),
              ImColor(g_textCol.x, g_textCol.y, g_textCol.z, alpha), title);

}

inline void MR_EndCard(ImDrawList *, ImVec2, ImVec2, float, float) {}

inline void MR_Checkbox(ImDrawList *dl, ImVec2 pos, const char *label, bool *v,
                        float alpha, float dt, bool input) {
  float sz = 22.0f;
  ImVec2 bp = pos + ImVec2(5, 0);
  std::string id = label;
  if (g_checkboxAnims.find(id) == g_checkboxAnims.end())
    g_checkboxAnims[id] = *v ? 1.f : 0.f;
  g_checkboxAnims[id] = MR_Damp(g_checkboxAnims[id], *v ? 1.f : 0.f, 15.f, dt);
  float anim = g_checkboxAnims[id];

  ImColor bgOff(10, 10, 10, (int)(255 * alpha));
  ImColor bgOn(g_accentColor.x, g_accentColor.y, g_accentColor.z, alpha);
  ImVec4 bg(MR_Lerp(bgOff.Value.x, bgOn.Value.x, anim),
            MR_Lerp(bgOff.Value.y, bgOn.Value.y, anim),
            MR_Lerp(bgOff.Value.z, bgOn.Value.z, anim), alpha);

  dl->AddRectFilled(bp, bp + ImVec2(sz, sz), ImGui::GetColorU32(bg),
                    7.f);

  if (anim < 0.5f)
    dl->AddRect(bp, bp + ImVec2(sz, sz), ImColor(40, 40, 40, (int)(180 * alpha * (1.0f - anim))),
                7.f);

  if (anim > 0.01f) {
    dl->PushClipRect(bp, bp + ImVec2(sz * anim, sz), true);
    dl->AddLine(bp + ImVec2(5, 11), bp + ImVec2(9, 16),
                ImColor(255, 255, 255, (int)(255 * alpha)), 2.5f);
    dl->AddLine(bp + ImVec2(9, 16), bp + ImVec2(17, 6),
                ImColor(255, 255, 255, (int)(255 * alpha)), 2.5f);
    dl->PopClipRect();
  }
  if (input) {
    ImGui::SetCursorScreenPos(bp);

    bool hiddenLabel = (label[0] == '#' && label[1] == '#');
    float hitW = sz + 4.0f;
    if (!hiddenLabel) {
      float labelWidth = ImGui::CalcTextSize(label).x;
      hitW = sz + 10.0f + labelWidth;
      if (hitW > 140.0f)
        hitW = 140.0f;
    }
    if (ImGui::InvisibleButton(label, ImVec2(hitW, sz)))
      *v = !(*v);
  }

  if (label[0] != '#' || label[1] != '#') {
    ImVec4 labelColor = ImColor(150, 150, 150, (int)(255 * alpha));
    MR_DrawText(dl, g_fontSmall, bp + ImVec2(sz + 10, 2), labelColor, label);
  }
}

inline void MR_Slider(ImDrawList *dl, ImVec2 pos, float w, const char *label,
                      float *v, float mn, float mx, float alpha, bool input,
                      const char *fmt = "%.0f") {

  static std::string g_editingSlider;
  static char g_editBuf[32] = {0};

  std::string id = "##sl_" + std::string(label);
  bool isEditing = (g_editingSlider == id);

  MR_DrawText(dl, g_fontSmall, pos, ImColor(190, 190, 190, (int)(255 * alpha)),
              label);

  char buf[32];
  sprintf(buf, fmt, *v);
  if (g_fontSmall)
    ImGui::PushFont(g_fontSmall);
  ImVec2 ts = ImGui::CalcTextSize(buf);
  if (g_fontSmall)
    ImGui::PopFont();

  ImVec2 valPos = pos + ImVec2(w - ts.x - 2, -2);
  float valW = ts.x + 10;
  float valH = ts.y + 4;

  if (isEditing) {

    ImGui::SetCursorScreenPos(valPos - ImVec2(30, 0));
    ImGui::PushItemWidth(valW + 30);
    if (g_fontSmall)
      ImGui::PushFont(g_fontSmall);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.08f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
                                ImGuiInputTextFlags_AutoSelectAll;
    bool committed = ImGui::InputText(("##edit_" + id).c_str(), g_editBuf,
                                       sizeof(g_editBuf), flags);
    bool focused = ImGui::IsItemActive();

    static std::string lastEditId;
    if (lastEditId != id) {
      lastEditId = id;
      ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    if (g_fontSmall)
      ImGui::PopFont();
    ImGui::PopItemWidth();

    if (committed) {
      float parsed = (float)atof(g_editBuf);
      if (parsed >= mn && parsed <= mx)
        *v = parsed;
      g_editingSlider.clear();
    } else if (!focused && ImGui::IsMouseClicked(0)) {
      g_editingSlider.clear();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
      g_editingSlider.clear();
  } else {
    MR_DrawText(dl, g_fontSmall, pos + ImVec2(w - ts.x, 0),
                ImColor(100, 100, 100, (int)(255 * alpha)), buf);

    if (input) {
      ImGui::SetCursorScreenPos(valPos);
      if (ImGui::InvisibleButton(("##vlck_" + id).c_str(), ImVec2(valW, valH))) {
        g_editingSlider = id;
        strcpy_s(g_editBuf, buf);
      }
    }
  }

  ImVec2 sp = pos + ImVec2(0, 22);
  float h = 12.f;
  dl->AddRectFilled(sp, sp + ImVec2(w, h),
                    ImColor(15, 15, 15, (int)(255 * alpha)),
                    h / 2.0f);

  if (g_sliderAnims.find(id) == g_sliderAnims.end())
    g_sliderAnims[id] = *v;

  if (input && !isEditing) {
    ImGui::SetCursorScreenPos(sp - ImVec2(0, 5));
    ImGui::InvisibleButton(label, ImVec2(w, h + 10));

    float step = 1.0f;
    if (strstr(fmt, ".2f"))
      step = 0.01f;
    else if (strstr(fmt, ".1f"))
      step = 0.1f;

    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
      if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        *v -= step;
      if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        *v += step;
    }

    if (ImGui::IsItemActive()) {
      ImGuiIO &io = ImGui::GetIO();
      if (io.KeyShift) {
        *v += io.MouseDelta.x * step * 0.5f;
      } else {
        float r = (io.MousePos.x - sp.x) / w;
        if (r < 0)
          r = 0;
        if (r > 1)
          r = 1;
        *v = mn + r * (mx - mn);
      }

      if (strstr(fmt, ".2f")) {
        *v = (float)(round((double)*v / 0.01) * 0.01);
      } else if (strstr(fmt, ".1f")) {
        *v = (float)(round((double)*v / 0.1) * 0.1);
      } else if (strstr(fmt, ".0f") || strstr(fmt, "d") || strstr(fmt, "i")) {
        *v = roundf(*v);
      }
    }

    if (*v < mn)
      *v = mn;
    if (*v > mx)
      *v = mx;
  }

  g_sliderAnims[id] =
      MR_Damp(g_sliderAnims[id], *v, 20.f, ImGui::GetIO().DeltaTime);
  float ratio = (g_sliderAnims[id] - mn) / (mx - mn);
  if (ratio < 0)
    ratio = 0;
  if (ratio > 1)
    ratio = 1;

  float fillW = w * ratio;
  if (fillW < h && fillW > 0.0f)
    fillW = h;
  if (ratio > 0.0f) {
    dl->AddRectFilled(
        sp, sp + ImVec2(fillW, h),
        ImColor(g_accentColor.x, g_accentColor.y, g_accentColor.z, alpha),
        h / 2.0f);
  }
}

inline bool MR_Dropdown(ImDrawList *dl, ImVec2 pos, float w, const char *label,
                        std::vector<std::string> opts, float alpha, float dt,
                        bool input, float screenBot) {
  std::string id = "##dd_" + std::string(label);
  bool changed = false;
  bool hasLabel = strlen(label) > 0 && std::string(label).find("##") != 0;
  if (hasLabel)
    MR_DrawText(dl, g_fontSmall, pos,
                ImColor(190, 190, 190, (int)(255 * alpha)), label);
  ImVec2 bp = pos + ImVec2(0, hasLabel ? 22.f : 0.f);
  float h = 32.f;
  if (g_dropdownSelections.find(id) == g_dropdownSelections.end())
    g_dropdownSelections[id] = 0;
  int ci = g_dropdownSelections[id];
  const char *cv =
      (ci >= 0 && ci < (int)opts.size()) ? opts[ci].c_str() : "None";

  dl->AddRectFilled(bp, bp + ImVec2(w, h), ImColor(4, 4, 4, (int)(255 * alpha)),
                    5.f);
  dl->AddRect(bp, bp + ImVec2(w, h), ImColor(25, 25, 25, (int)(255 * alpha)),
              5.f);
  if (g_fontSmall)
    ImGui::PushFont(g_fontSmall);
  dl->AddText(bp + ImVec2(8, 4),
              ImColor(g_textCol.x, g_textCol.y, g_textCol.z, alpha), cv);
  if (g_fontSmall)
    ImGui::PopFont();
  ImVec2 ap = bp + ImVec2(w - 15, h / 2 - 2);
  dl->AddTriangleFilled(ap, ap + ImVec2(8, 0), ap + ImVec2(4, 5),
                        ImColor(150, 150, 150, (int)(255 * alpha)));
  if (input) {
    ImGui::SetCursorScreenPos(bp);
    if (ImGui::InvisibleButton(id.c_str(), ImVec2(w, h)))
      g_openDropdown = (g_openDropdown == id) ? "" : id;
  }
  bool isOpen = (g_openDropdown == id);
  if (g_dropdownAnims.find(id) == g_dropdownAnims.end())
    g_dropdownAnims[id] = 0.f;
  g_dropdownAnims[id] =
      MR_Damp(g_dropdownAnims[id], isOpen ? 1.f : 0.f, 15.f, dt);
  float anim = g_dropdownAnims[id];
  if (anim > 0.01f) {
    float ih = 28.f, th = opts.size() * ih;

    float maxVisH = 8.f * ih;
    float visH = (th > maxVisH) ? maxVisH : th;
    float ch = visH * anim;
    bool up = (bp.y + h + visH > screenBot - 10);
    ImVec2 lp = up ? bp - ImVec2(0, ch + 2) : bp + ImVec2(0, h + 2);
    ImDrawList *fg = ImGui::GetForegroundDrawList();
    fg->PushClipRect(lp, lp + ImVec2(w, ch), true);

    static std::map<std::string, float> s_scrollOffsets;
    if (s_scrollOffsets.find(id) == s_scrollOffsets.end())
      s_scrollOffsets[id] = 0.f;
    float &scrollOff = s_scrollOffsets[id];

    if (isOpen && anim > 0.8f) {
      ImVec2 mpos = ImGui::GetIO().MousePos;
      if (mpos.x >= lp.x && mpos.x <= lp.x + w && mpos.y >= lp.y &&
          mpos.y <= lp.y + ch) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.f) {
          scrollOff -= wheel * ih * 2.f;
          float maxScroll = th - visH;
          if (scrollOff < 0.f)
            scrollOff = 0.f;
          if (scrollOff > maxScroll)
            scrollOff = maxScroll;
        }
      }
    }
    if (!isOpen)
      scrollOff = 0.f;

    float maxScroll = th - visH;
    if (maxScroll < 0.f)
      maxScroll = 0.f;
    if (scrollOff > maxScroll)
      scrollOff = maxScroll;
    fg->AddRectFilled(lp, lp + ImVec2(w, visH),
                      ImColor(4, 4, 4, (int)(255 * alpha)), 5.f);
    fg->AddRect(lp, lp + ImVec2(w, visH),
                ImColor(25, 25, 25, (int)(255 * alpha)), 5.f);
    if (g_fontSmall)
      ImGui::PushFont(g_fontSmall);
    for (int i = 0; i < (int)opts.size(); i++) {
      ImVec2 ip = lp + ImVec2(0, i * ih - scrollOff);

      if (ip.y + ih < lp.y || ip.y > lp.y + ch)
        continue;
      bool hov = false;
      if (isOpen && anim > 0.8f) {
        ImGui::SetCursorScreenPos(ip);
        if (ImGui::InvisibleButton(("##o_" + id + std::to_string(i)).c_str(),
                                   ImVec2(w, ih))) {
          g_dropdownSelections[id] = i;
          g_openDropdown = "";
          changed = true;
        }
        hov = ImGui::IsItemHovered();
      }
      ImColor tc = (hov || i == ci)
                       ? ImColor(g_accentColor.x, g_accentColor.y,
                                 g_accentColor.z, alpha)
                       : ImColor(180, 180, 180, (int)(255 * alpha));
      if (hov)
        fg->AddRectFilled(ip, ip + ImVec2(w, ih),
                          ImColor(15, 15, 15, (int)(255 * alpha)));
      fg->AddText(ip + ImVec2(10, 4), tc, opts[i].c_str());
    }
    if (g_fontSmall)
      ImGui::PopFont();

    if (th > visH && anim > 0.5f) {
      float scrollRatio = scrollOff / maxScroll;
      float barH = (visH / th) * visH;
      float barY = lp.y + scrollRatio * (visH - barH);
      fg->AddRectFilled(ImVec2(lp.x + w - 3, barY),
                        ImVec2(lp.x + w - 1, barY + barH),
                        ImColor(100, 100, 100, (int)(150 * alpha)), 1.f);
    }
    fg->PopClipRect();
  }
  return changed;
}

inline void MR_RenderColorPicker(ImDrawList *dl, ImVec2 pos, ImVec2 size,
                                 float alpha) {
  float bh = 260.f;
  ImVec2 bs(size.x, bh);
  int cpBgAlpha = (int)(255.0f * alpha);
  dl->AddRectFilled(pos, pos + bs, ImColor(8, 8, 12, cpBgAlpha), 4.f);
  dl->AddRectFilled(
      pos, ImVec2(pos.x + bs.x, pos.y + 1),
      ImColor(g_accentColor.x, g_accentColor.y, g_accentColor.z, 0.4f * alpha));
  dl->AddRect(pos, pos + bs, ImColor(60, 60, 70, (int)(180 * alpha)), 4.f);
  if (g_fontHeader)
    ImGui::PushFont(g_fontHeader);
  dl->AddText(pos + ImVec2(15, 15), ImColor(255, 255, 255, (int)(255 * alpha)),
              "Edit Color");
  if (g_fontHeader)
    ImGui::PopFont();
  ImVec2 cp = pos + ImVec2(bs.x - 30, 15);
  ImGui::SetCursorScreenPos(cp);
  if (ImGui::InvisibleButton("##cpX", ImVec2(16, 16)))
    g_showColorPicker = false;
  dl->AddLine(cp, cp + ImVec2(16, 16),
              ImColor(150, 150, 150, (int)(255 * alpha)));
  dl->AddLine(cp + ImVec2(16, 0), cp + ImVec2(0, 16),
              ImColor(150, 150, 150, (int)(255 * alpha)));
  ImGui::SetCursorScreenPos(pos + ImVec2(15, 50));
  ImGui::PushItemWidth(bs.x - 30);
  ImGui::ColorPicker4(
      "##picker",
      (float *)(g_colorEditTarget ? g_colorEditTarget : &g_accentColor),
      ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
          ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

inline void MR_HandleDualScroll(float height, float contentHL, float contentHR,
                                float &scrollL, float &scrollR, bool input) {
  float maxL = contentHL > height ? contentHL - height : 0.0f;
  float maxR = contentHR > height ? contentHR - height : 0.0f;
  float maxScroll = maxL > maxR ? maxL : maxR;

  if (input && g_openDropdown.empty() && ImGui::GetIO().MouseWheel != 0.0f) {
    float delta = -ImGui::GetIO().MouseWheel * 35.0f;
    scrollL += delta;
  }

  if (scrollL < 0.0f)
    scrollL = 0.0f;
  if (scrollL > maxScroll)
    scrollL = maxScroll;

  scrollR = scrollL;
}

inline const char *MR_GetKeyName(int k) {
  if (k == VK_LBUTTON)
    return "LMB";
  if (k == VK_RBUTTON)
    return "RMB";
  if (k == VK_MBUTTON)
    return "MMB";
  if (k == VK_XBUTTON1)
    return "Mouse 4";
  if (k == VK_XBUTTON2)
    return "Mouse 5";
  if (k == VK_CONTROL || k == VK_LCONTROL || k == VK_RCONTROL)
    return "Ctrl";
  if (k == VK_SHIFT || k == VK_LSHIFT || k == VK_RSHIFT)
    return "Shift";
  if (k == VK_MENU || k == VK_LMENU || k == VK_RMENU)
    return "Alt";
  if (k == VK_SPACE)
    return "Space";
  if (k == VK_INSERT)
    return "Insert";
  if (k == VK_DELETE)
    return "Delete";
  if (k == VK_HOME)
    return "Home";
  if (k == VK_END)
    return "End";
  if (k == VK_TAB)
    return "Tab";
  if (k == VK_ESCAPE)
    return "Esc";
  if (k >= VK_F1 && k <= VK_F12) {
    static char fbuf[8];
    sprintf(fbuf, "F%d", k - VK_F1 + 1);
    return fbuf;
  }

  static char buf[32];
  if ((k >= 'A' && k <= 'Z') || (k >= '0' && k <= '9')) {
    sprintf(buf, "%c", (char)k);
    return buf;
  }
  sprintf(buf, "0x%X", k);
  return buf;
}

static int g_keyWaitFrames = 0;

inline void MR_Hotkey(ImDrawList *dl, ImVec2 pos, float w, const char *label,
                      int *k, float alpha, float dt, bool input) {
  MR_DrawText(dl, g_fontSmall, pos, ImColor(190, 190, 190, (int)(255 * alpha)),
              label);
  ImVec2 bp = pos + ImVec2(0, 22);
  float h = 32.f;
  std::string id = "##hk_" + std::string(label);

  bool isWaiting = (g_waitingForKey && g_keyTarget == k);

  ImColor bgCol = isWaiting ? ImColor(g_accentColor.x, g_accentColor.y,
                                      g_accentColor.z, alpha * 0.4f)
                            : ImColor(4, 4, 4, (int)(255 * alpha));
  dl->AddRectFilled(bp, bp + ImVec2(w, h), bgCol, 5.f);
  dl->AddRect(bp, bp + ImVec2(w, h), ImColor(25, 25, 25, (int)(255 * alpha)),
              5.f);

  const char *kn = isWaiting ? "[Press Key]" : MR_GetKeyName(*k);
  if (g_fontSmall)
    ImGui::PushFont(g_fontSmall);
  ImVec2 ts = ImGui::CalcTextSize(kn);
  dl->AddText(bp + ImVec2((w - ts.x) * 0.5f, 4),
              ImColor(g_textCol.x, g_textCol.y, g_textCol.z, alpha), kn);
  if (g_fontSmall)
    ImGui::PopFont();

  if (input) {
    ImGui::SetCursorScreenPos(bp);
    if (ImGui::InvisibleButton(id.c_str(), ImVec2(w, h))) {
      g_waitingForKey = true;
      g_keyTarget = k;
      g_keyId = id;
      g_keyWaitFrames = 2;
    }
  }

  if (isWaiting) {

    if (g_keyWaitFrames > 0) {
      g_keyWaitFrames--;
      return;
    }

    for (int i = 1; i < 256; i++) {
      if (i == VK_LWIN || i == VK_RWIN)
        continue;
      if (i == VK_LBUTTON)
        continue;
      if (GetAsyncKeyState(i) & 0x8000) {
        if (i == VK_ESCAPE) {
          g_waitingForKey = false;
          g_keyTarget = nullptr;
        } else {
          *k = i;
          g_waitingForKey = false;
          g_keyTarget = nullptr;
        }
        break;
      }
    }
  }
}

inline void MR_HotkeyInline(ImDrawList *dl, ImVec2 pos, float w,
                             const char *id_str, int *k, float alpha, float dt,
                             bool input) {
  float h = 32.f;
  std::string id = std::string("##hki_") + id_str;

  bool isWaiting = (g_waitingForKey && g_keyTarget == k);

  ImColor bgCol = isWaiting ? ImColor(g_accentColor.x, g_accentColor.y,
                                      g_accentColor.z, alpha * 0.4f)
                            : ImColor(4, 4, 4, (int)(255 * alpha));
  dl->AddRectFilled(pos, pos + ImVec2(w, h), bgCol, 5.f);
  dl->AddRect(pos, pos + ImVec2(w, h), ImColor(25, 25, 25, (int)(255 * alpha)),
              5.f);

  const char *kn = isWaiting ? "[...]" : MR_GetKeyName(*k);
  if (g_fontSmall)
    ImGui::PushFont(g_fontSmall);
  ImVec2 ts = ImGui::CalcTextSize(kn);
  dl->AddText(pos + ImVec2((w - ts.x) * 0.5f, 4),
              ImColor(g_textCol.x, g_textCol.y, g_textCol.z, alpha), kn);
  if (g_fontSmall)
    ImGui::PopFont();

  if (input) {
    ImGui::SetCursorScreenPos(pos);
    if (ImGui::InvisibleButton(id.c_str(), ImVec2(w, h))) {
      g_waitingForKey = true;
      g_keyTarget = k;
      g_keyId = id;
      g_keyWaitFrames = 2;
    }
  }

  if (isWaiting) {
    if (g_keyWaitFrames > 0) {
      g_keyWaitFrames--;
      return;
    }
    for (int i = 1; i < 256; i++) {
      if (i == VK_LWIN || i == VK_RWIN)
        continue;
      if (i == VK_LBUTTON)
        continue;
      if (GetAsyncKeyState(i) & 0x8000) {
        if (i == VK_ESCAPE) {
          g_waitingForKey = false;
          g_keyTarget = nullptr;
        } else {
          *k = i;
          g_waitingForKey = false;
          g_keyTarget = nullptr;
        }
        break;
      }
    }
  }
}
