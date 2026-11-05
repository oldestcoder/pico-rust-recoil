#pragma once
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_internal.h"
#include <cmath>
#include <map>

namespace menu_colors {
inline ImVec4 background = ImColor(4, 4, 4, 255);
inline ImVec4 layout = ImColor(4, 4, 4, 255);
inline ImVec4 child = ImColor(8, 8, 8, 255);
inline ImVec4 text = ImColor(234, 234, 234, 255);
inline ImVec4 white = ImColor(255, 255, 255, 255);
inline ImVec4 accent = ImColor(255, 20, 20, 255);
inline ImVec4 accent_dim = ImColor(80, 10, 10, 255);
inline ImVec4 accent_bright = ImColor(255, 60, 60, 255);
inline ImVec4 black = ImColor(0, 0, 0, 255);
inline ImVec4 lightchild = ImColor(26, 26, 28, 255);
inline ImVec4 selectable = ImColor(28, 28, 30, 255);
inline ImVec4 button = ImColor(24, 24, 26, 255);
inline ImVec4 button_hover = ImColor(35, 25, 25, 255);
inline ImVec4 button_active = ImColor(45, 15, 15, 255);

namespace bg {
inline ImVec2 size = ImVec2(660, 420);
inline float rounding = 12.f;
}
}

namespace menu_animations {
inline float easing(float &value, float target, float speed) {
  value =
      ImLerp(value, target,
             ImGui::GetIO().Framerate > 0.0f ? speed / ImGui::GetIO().Framerate
                                             : speed);
  return value;
}

struct checkbox_anim {
  float alpha;
  float check_anim;
  float glow_anim;
};

struct slider_anim {
  float hover_alpha;
  float grab_anim;
  float active_anim;
  float value_anim;
  float glow_pulse;
};

struct combo_anim {
  float open_anim;
};
}

namespace menu_checkbox {
inline bool Checkbox(const char *label, bool *v) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

  const float square_sz = ImGui::GetFrameHeight();
  const ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(
      pos,
      ImVec2(pos.x + square_sz +
                 (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x
                                      : 0.0f),
             pos.y + label_size.y + style.FramePadding.y * 2.0f));
  ImGui::ItemSize(total_bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(total_bb, id))
    return false;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
  if (pressed)
    *v = !(*v);

  static std::map<ImGuiID, menu_animations::checkbox_anim> anim;
  auto it_anim = anim.find(id);
  if (it_anim == anim.end()) {
    anim.insert({id, {0.0f, 0.0f, 0.0f}});
    it_anim = anim.find(id);
  }

  float target_alpha = (hovered || *v) ? 1.0f : 0.6f;
  float target_check = *v ? 1.0f : 0.0f;
  menu_animations::easing(it_anim->second.alpha, target_alpha, 10.0f);
  menu_animations::easing(it_anim->second.check_anim, target_check, 12.0f);

  if (*v) {
    it_anim->second.glow_anim += ImGui::GetIO().DeltaTime * 3.0f;
  } else {
    it_anim->second.glow_anim = 0.0f;
  }
  float glow_alpha =
      *v ? (0.15f + 0.1f * sinf(it_anim->second.glow_anim)) : 0.0f;

  const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));

  if (glow_alpha > 0.01f) {
    ImU32 glow_col =
        ImGui::GetColorU32(ImVec4(menu_colors::accent.x, menu_colors::accent.y,
                                  menu_colors::accent.z, glow_alpha));
    ImVec2 glow_expand(4.0f, 4.0f);
    window->DrawList->AddRectFilled(
        ImVec2(check_bb.Min.x - glow_expand.x, check_bb.Min.y - glow_expand.y),
        ImVec2(check_bb.Max.x + glow_expand.x, check_bb.Max.y + glow_expand.y),
        glow_col, 5.0f);
  }

  ImU32 bg_col = ImGui::GetColorU32(
      ImVec4(menu_colors::lightchild.x, menu_colors::lightchild.y,
             menu_colors::lightchild.z, it_anim->second.alpha));
  window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, bg_col, 3.0f);
  window->DrawList->AddRect(
      check_bb.Min, check_bb.Max,
      ImColor(40, 40, 40, (int)(255 * it_anim->second.alpha)), 3.0f);

  if (it_anim->second.check_anim > 0.01f) {
    const float pad = ImMax(1.0f, truncf(square_sz / 6.0f));

    ImVec2 check_p1 =
        ImVec2(check_bb.Min.x + pad + 2.0f, check_bb.Min.y + square_sz * 0.5f);
    ImVec2 check_p2 =
        ImVec2(check_bb.Min.x + square_sz * 0.45f, check_bb.Max.y - pad - 2.0f);
    ImVec2 check_p3 =
        ImVec2(check_bb.Max.x - pad - 1.0f, check_bb.Min.y + pad + 1.0f);

    ImU32 check_col = ImGui::GetColorU32(
        ImVec4(menu_colors::accent.x, menu_colors::accent.y,
               menu_colors::accent.z, it_anim->second.check_anim));

    window->DrawList->AddLine(check_p1, check_p2, check_col, 2.5f);
    window->DrawList->AddLine(check_p2, check_p3, check_col, 2.5f);
  }

  if (label_size.x > 0.0f) {
    ImGui::RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x,
                             check_bb.Min.y + style.FramePadding.y),
                      label);
  }

  return pressed;
}
}

namespace menu_slider {

inline bool SliderFloat(const char *label, float *v, float v_min, float v_max,
                        const char *format = "%.1f") {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const float w = ImGui::CalcItemWidth();
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

  char value_buf[64];
  snprintf(value_buf, sizeof(value_buf), format, *v);
  const ImVec2 value_size = ImGui::CalcTextSize(value_buf, NULL, true);

  const ImVec2 pos = window->DC.CursorPos;
  const float track_height = 10.0f;
  const float grab_radius = 7.0f;

  const float text_height = ImMax(label_size.y, value_size.y);
  const float slider_start_y = pos.y + text_height + style.ItemInnerSpacing.y;
  const float total_height =
      text_height + style.ItemInnerSpacing.y + grab_radius * 2.0f;

  const ImRect frame_bb(ImVec2(pos.x, slider_start_y),
                        ImVec2(pos.x + w, slider_start_y + grab_radius * 2.0f));
  const ImRect total_bb(pos, ImVec2(pos.x + w, pos.y + total_height));

  ImGui::ItemSize(total_bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(total_bb, id))
    return false;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);

  static std::map<ImGuiID, menu_animations::slider_anim> anim;
  auto it = anim.find(id);
  if (it == anim.end()) {
    float norm = (*v - v_min) / (v_max - v_min);
    anim.insert({id, {0.0f, 0.0f, 0.0f, norm, 0.0f}});
    it = anim.find(id);
  }

  float target_hover = hovered ? 1.0f : 0.0f;
  float target_active = held ? 1.0f : 0.0f;
  menu_animations::easing(it->second.hover_alpha, target_hover, 10.0f);
  menu_animations::easing(it->second.active_anim, target_active, 12.0f);

  float target_grab_size = held ? 1.0f : (hovered ? 0.6f : 0.0f);
  menu_animations::easing(it->second.grab_anim, target_grab_size, 10.0f);

  if (held) {
    it->second.glow_pulse += ImGui::GetIO().DeltaTime * 4.0f;
  } else {
    menu_animations::easing(it->second.glow_pulse, 0.0f, 5.0f);
  }

  if (held) {
    float mouse_x = g.IO.MousePos.x;
    float norm =
        ImClamp((mouse_x - frame_bb.Min.x) / (frame_bb.Max.x - frame_bb.Min.x),
                0.0f, 1.0f);
    *v = v_min + norm * (v_max - v_min);
    pressed = true;
  }

  float target_norm = ImClamp((*v - v_min) / (v_max - v_min), 0.0f, 1.0f);
  menu_animations::easing(it->second.value_anim, target_norm, 15.0f);
  float anim_norm = it->second.value_anim;

  float track_y = frame_bb.Min.y + frame_bb.GetHeight() * 0.5f;
  ImVec2 track_min(frame_bb.Min.x, track_y - track_height * 0.5f);
  ImVec2 track_max(frame_bb.Max.x, track_y + track_height * 0.5f);

  if (label_size.x > 0.0f) {
    ImGui::RenderText(ImVec2(pos.x, pos.y), label);
  }

  ImGui::RenderText(ImVec2(frame_bb.Max.x - value_size.x, pos.y), value_buf);

  window->DrawList->AddRectFilled(
      track_min, track_max, ImColor(20, 20, 22, 255), track_height * 0.5f);

  float fill_x = frame_bb.Min.x + anim_norm * (frame_bb.Max.x - frame_bb.Min.x);
  if (anim_norm > 0.001f) {
    ImU32 fill_col =
        ImGui::GetColorU32(ImVec4(menu_colors::accent.x, menu_colors::accent.y,
                                  menu_colors::accent.z, 255));
    window->DrawList->AddRectFilled(track_min, ImVec2(fill_x, track_max.y),
                                    fill_col, track_height * 0.5f);
  }

  float grab_w = 4.0f + it->second.grab_anim * 2.0f;
  float grab_h = 10.0f + it->second.grab_anim * 4.0f;
  ImVec2 grab_min(fill_x - grab_w * 0.5f, track_y - grab_h * 0.5f);
  ImVec2 grab_max(fill_x + grab_w * 0.5f, track_y + grab_h * 0.5f);

  float glow_intensity =
      it->second.hover_alpha * 0.3f + it->second.active_anim * 0.2f +
      sinf(it->second.glow_pulse) * 0.1f * it->second.active_anim;

  if (glow_intensity > 0.01f) {
    ImU32 glow_col =
        ImGui::GetColorU32(ImVec4(menu_colors::accent.x, menu_colors::accent.y,
                                  menu_colors::accent.z, glow_intensity));
    window->DrawList->AddRectFilled(
        ImVec2(grab_min.x - 2.0f, grab_min.y - 4.0f),
        ImVec2(grab_max.x + 2.0f, grab_max.y + 4.0f), glow_col, 2.0f);
  }

  return pressed;
}

inline bool SliderInt(const char *label, int *v, int v_min, int v_max,
                      const char *format = "%d") {
  float v_f = (float)*v;
  bool changed = SliderFloat(label, &v_f, (float)v_min, (float)v_max, format);
  *v = (int)v_f;
  return changed;
}

}

namespace menu_tabs {
struct tab_anim {
  float animation;
  float text_alpha;
  float glow;
};

inline bool Tab(const char *label, const char *icon, bool selected,
                const ImVec2 &size_arg) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

  ImVec2 pos = window->DC.CursorPos;
  ImVec2 size =
      ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f,
                          label_size.y + style.FramePadding.y * 2.0f);

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ImGui::ItemSize(size, style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, id))
    return false;

  ImGuiButtonFlags flags = ImGuiButtonFlags_PressedOnClick;
  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

  static std::map<ImGuiID, tab_anim> anim;
  auto it_anim = anim.find(id);
  if (it_anim == anim.end()) {
    anim.insert({id, {0.0f, 0.6f, 0.0f}});
    it_anim = anim.find(id);
  }

  float target_anim = selected ? 1.0f : 0.0f;
  float target_alpha = selected ? 1.0f : (hovered ? 0.8f : 0.6f);
  menu_animations::easing(it_anim->second.animation, target_anim, 8.0f);
  menu_animations::easing(it_anim->second.text_alpha, target_alpha, 8.0f);

  float target_glow = (hovered || selected) ? 1.0f : 0.0f;
  menu_animations::easing(it_anim->second.glow, target_glow, 8.0f);

  if (it_anim->second.glow > 0.01f) {
    ImU32 glow_col = ImGui::GetColorU32(
        ImVec4(menu_colors::accent.x, menu_colors::accent.y,
               menu_colors::accent.z, it_anim->second.glow * 0.08f));
    window->DrawList->AddRectFilled(bb.Min, bb.Max, glow_col,
                                    menu_colors::bg::rounding);
  }

  ImU32 bg_col = ImGui::GetColorU32(
      ImVec4(menu_colors::accent.x, menu_colors::accent.y,
             menu_colors::accent.z, it_anim->second.animation * 0.15f));
  window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col,
                                  menu_colors::bg::rounding);

  if (it_anim->second.animation > 0.01f) {
    window->DrawList->AddRectFilled(
        ImVec2(bb.Min.x, bb.Max.y - 2), bb.Max,
        ImGui::GetColorU32(ImVec4(menu_colors::accent.x, menu_colors::accent.y,
                                  menu_colors::accent.z,
                                  it_anim->second.animation)),
        menu_colors::bg::rounding, ImDrawFlags_RoundCornersBottom);
  }

  ImVec4 text_color = ImVec4(menu_colors::text.x, menu_colors::text.y,
                             menu_colors::text.z, it_anim->second.text_alpha);

  ImGui::PushStyleColor(ImGuiCol_Text, text_color);

  ImGui::RenderTextClipped(bb.Min, bb.Max, label, NULL, &label_size,
                           ImVec2(0.5f, 0.5f), &bb);
  ImGui::PopStyleColor();

  return pressed;
}
}
