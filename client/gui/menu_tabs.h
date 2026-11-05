#pragma once
#include "menu_globals.h"

void RenderRecoilTab(ImDrawList *dl, ImVec2 pos, float w, float alpha, float dt,
                     bool input, ImVec2 winPos, ImVec2 winSize, int subTab);
void RenderTuningTab(ImDrawList *dl, ImVec2 pos, float w, float alpha, float dt,
                     bool input, ImVec2 winPos, ImVec2 winSize, int subTab);
void RenderSettingsTab(ImDrawList *dl, ImVec2 pos, float w, float alpha,
                       float dt, bool input, ImVec2 winPos, ImVec2 winSize,
                       int subTab);
void RenderKeybindsTab(ImDrawList *dl, ImVec2 pos, float w, float alpha,
                       float dt, bool input, ImVec2 winPos, ImVec2 winSize,
                       int subTab);
