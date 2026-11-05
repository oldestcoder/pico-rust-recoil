#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu_render.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_internal.h"
#include "../src/game_data.h"
#include "menu_globals.h"
#include "menu_styles.hpp"
#include "menu_tabs.h"
#include <cstdio>
#include <d3d11.h>
#include <dwmapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

static HWND g_menuHwnd = NULL;
static ID3D11Device *g_pd3dDevice = NULL;
static ID3D11DeviceContext *g_pd3dContext = NULL;
static IDXGISwapChain *g_pSwapChain = NULL;
static ID3D11RenderTargetView *g_rtv = NULL;
static bool g_dragging = false;
static POINT g_dragStart = {};
static float g_menuAlpha = 0.0f;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM,
                                                             LPARAM);

#include "IconsFontAwesome5.h"
#include "fa_solid_900.h"
#include "logo_data.h"

static ID3D11ShaderResourceView *g_logoTexture = nullptr;

static void LoadLogoTexture(ID3D11Device *device) {
  if (g_logoTexture || !device)
    return;
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = logo_width;
  desc.Height = logo_height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA subResource;
  subResource.pSysMem = logo_pixels;
  subResource.SysMemPitch = logo_width * 4;

  ID3D11Texture2D *pTexture = NULL;
  device->CreateTexture2D(&desc, &subResource, &pTexture);

  if (pTexture) {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    device->CreateShaderResourceView(pTexture, &srvDesc, &g_logoTexture);
    pTexture->Release();
  }
}

static LRESULT WINAPI MenuWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;
  switch (msg) {
  case WM_SIZE:
    if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
      if (g_rtv) {
        g_rtv->Release();
        g_rtv = NULL;
      }
      g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam),
                                  DXGI_FORMAT_UNKNOWN, 0);
      ID3D11Texture2D *buf;
      g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&buf));
      g_pd3dDevice->CreateRenderTargetView(buf, NULL, &g_rtv);
      buf->Release();
    }
    return 0;
  case WM_LBUTTONDOWN: {
    POINT pt;
    GetCursorPos(&pt);
    RECT rc;
    GetWindowRect(hWnd, &rc);
    if (pt.y - rc.top < 40) {
      g_dragging = true;
      g_dragStart = pt;
      g_dragStart.x -= rc.left;
      g_dragStart.y -= rc.top;
      SetCapture(hWnd);
    }
    break;
  }
  case WM_MOUSEMOVE:
    if (g_dragging) {
      POINT pt;
      GetCursorPos(&pt);
      SetWindowPos(hWnd, NULL, pt.x - g_dragStart.x, pt.y - g_dragStart.y, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER);
    }
    break;
  case WM_LBUTTONUP:
    if (g_dragging) {
      g_dragging = false;
      ReleaseCapture();
    }
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool InitMainMenu() {
  printf("[+] InitMainMenu: Setting up...\n");
  fflush(stdout);

  WNDCLASSEXA wc = {sizeof(WNDCLASSEXA),   CS_CLASSDC, MenuWndProc, 0,    0,
                    GetModuleHandle(NULL), NULL,       NULL,        NULL, NULL,
                    "UsbMouseBridgeWnd",     NULL};
  RegisterClassExA(&wc);

  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);
  int winW = 660, winH = 420;

  g_menuHwnd = CreateWindowExA(0, wc.lpszClassName,
                               "USB Mouse Bridge", WS_POPUP, (screenW - winW) / 2,
                               (screenH - winH) / 2, winW, winH, NULL, NULL,
                               wc.hInstance, NULL);
  if (!g_menuHwnd) {
    printf("[!] InitMainMenu: CreateWindow failed\n");
    fflush(stdout);
    return false;
  }

  HRGN hRgn = CreateRoundRectRgn(0, 0, winW, winH, 16, 16);
  SetWindowRgn(g_menuHwnd, hRgn, TRUE);

  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate = {60, 1};
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = g_menuHwnd;
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  D3D_FEATURE_LEVEL fl;
  D3D_FEATURE_LEVEL fla[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, fla, 2, D3D11_SDK_VERSION, &sd,
      &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dContext);
  if (FAILED(hr) || !g_pSwapChain || !g_pd3dDevice || !g_pd3dContext) {
    printf("[!] InitMainMenu: D3D11 device creation failed (0x%08lX)\n", hr);
    fflush(stdout);
    return false;
  }

  ID3D11Texture2D *buf;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&buf));
  g_pd3dDevice->CreateRenderTargetView(buf, NULL, &g_rtv);
  buf->Release();

  LoadLogoTexture(g_pd3dDevice);

  ShowWindow(g_menuHwnd, SW_SHOWDEFAULT);
  UpdateWindow(g_menuHwnd);

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = NULL;
  ImGui_ImplWin32_Init(g_menuHwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);

  char winFolder[512];
  GetWindowsDirectoryA(winFolder, 512);
  std::string fontDir = std::string(winFolder) + "\\Fonts\\";

  g_fontSmall =
      io.Fonts->AddFontFromFileTTF((fontDir + "segoeuib.ttf").c_str(), 15.5f);
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  io.Fonts->AddFontFromMemoryTTF((void *)fa_solid_data, fa_solid_size, 14.5f,
                                 &icons_config, icons_ranges);

  g_fontHeader =
      io.Fonts->AddFontFromFileTTF((fontDir + "segoeuib.ttf").c_str(), 17.0f);
  if (!g_fontHeader)
    g_fontHeader =
        io.Fonts->AddFontFromFileTTF((fontDir + "segoeui.ttf").c_str(), 17.0f);
  io.Fonts->AddFontFromMemoryTTF((void *)fa_solid_data, fa_solid_size, 16.0f,
                                 &icons_config, icons_ranges);

  g_fontTiny =
      io.Fonts->AddFontFromFileTTF((fontDir + "segoeuib.ttf").c_str(), 13.5f);

  g_fontDisplay =
      io.Fonts->AddFontFromFileTTF((fontDir + "segoeuib.ttf").c_str(), 36.0f);
  if (!g_fontDisplay)
    g_fontDisplay =
        io.Fonts->AddFontFromFileTTF((fontDir + "segoeui.ttf").c_str(), 36.0f);

  g_menuFont = g_fontSmall;
  g_menuFontSmall = g_fontTiny;
  g_menuFontLarge = g_fontHeader;

  if (!g_fontSmall)
    g_fontSmall = io.Fonts->AddFontDefault();
  if (!g_fontHeader)
    g_fontHeader = io.Fonts->AddFontDefault();
  if (!g_fontTiny)
    g_fontTiny = io.Fonts->AddFontDefault();
  if (!g_fontDisplay)
    g_fontDisplay = io.Fonts->AddFontDefault();

  printf("[+] InitMainMenu: Complete\n");
  fflush(stdout);
  return true;
}

void RenderMainMenu() {
  int menuToggleKey = VK_INSERT;

  while (g_running) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
        g_running = false;
    }
    if (!g_running)
      break;

    if (IsIconic(g_menuHwnd)) {
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();
      ImGui::EndFrame();
      Sleep(50);
      continue;
    }

    float dt = ImGui::GetIO().DeltaTime;

    {
      static bool toggleKeyWasDown = false;
      int toggleVk =
          (g_gameState.hideMenuKey != 0) ? g_gameState.hideMenuKey : VK_INSERT;
      bool toggleKeyDown = (GetAsyncKeyState(toggleVk) & 0x8000) != 0;
      if (toggleKeyDown && !toggleKeyWasDown) {
        g_menuVisible = !g_menuVisible;
        if (g_menuVisible)
          ShowWindow(g_menuHwnd, SW_SHOWNOACTIVATE);
        else
          ShowWindow(g_menuHwnd, SW_HIDE);
      }
      toggleKeyWasDown = toggleKeyDown;
    }

    {
      static bool recoilToggleWasDown = false;
      if (g_gameState.toggleKey != 0) {
        bool down = (GetAsyncKeyState(g_gameState.toggleKey) & 0x8000) != 0;
        if (down && !recoilToggleWasDown) {
          g_gameState.recoilEnabled = !g_gameState.recoilEnabled;
        }
        recoilToggleWasDown = down;
      }
    }

    {

      static const char *weaponList[] = {"AK-47","LR300","MP5A4","Custom SMG",
        "Thompson","HMLMG","M249","Semi Auto Rifle","Revolver",
        "Semi Automatic Pistol","M92","M39","Python","Nail Gun"};
      static const char *scopeList[] = {"None","Holo","Handmade","8x"};
      static const char *barrelList[] = {"None","Silencer","Muzzle Boost","Muzzle Brake"};

      auto syncDropdown = [](const char *ddKey, const char **list, int count, const std::string &val) {
        for (int i = 0; i < count; i++) {
          if (val == list[i]) {
            g_dropdownSelections[ddKey] = i;
            return;
          }
        }
      };

      static std::map<int, bool> customKeyWasDown;
      for (auto &cb : g_gameState.customKeybinds) {
        if (!cb.enabled || cb.keybind == 0)
          continue;
        bool down = (GetAsyncKeyState(cb.keybind) & 0x8000) != 0;
        bool wasDown = customKeyWasDown[cb.keybind];
        bool triggered = false;

        if (cb.type == "toggle") {
          if (down && !wasDown)
            triggered = true;
        } else {
          triggered = down;
        }

        if (triggered) {
          if (cb.actionType == "option") {
            if (cb.actionValue == "Master Toggle")
              g_gameState.recoilEnabled = !g_gameState.recoilEnabled;
            else if (cb.actionValue == "Hide Menu") {
              g_menuVisible = !g_menuVisible;
              if (g_menuVisible)
                ShowWindow(g_menuHwnd, SW_SHOWNOACTIVATE);
              else
                ShowWindow(g_menuHwnd, SW_HIDE);
            } else if (cb.actionValue == "Hipfire Toggle")
              g_gameState.hipfireEnabled = !g_gameState.hipfireEnabled;
          } else if (cb.actionType == "weapon") {
            g_gameState.currentWeapon = cb.actionValue;
            syncDropdown("##dd_Weapon", weaponList, 14, cb.actionValue);
            updateScaledPattern();
          } else if (cb.actionType == "attachment") {
            g_gameState.currentScope = cb.actionValue;
            syncDropdown("##dd_Scope", scopeList, 4, cb.actionValue);
            updateScaledPattern();
          } else if (cb.actionType == "barrel") {
            g_gameState.currentBarrel = cb.actionValue;
            syncDropdown("##dd_Barrel", barrelList, 4, cb.actionValue);
            updateScaledPattern();
          } else if (cb.actionType == "preset") {
            g_gameState.currentWeapon = cb.presetWeapon;
            g_gameState.currentScope = cb.presetScope;
            g_gameState.currentBarrel = cb.presetBarrel;
            syncDropdown("##dd_Weapon", weaponList, 14, cb.presetWeapon);
            syncDropdown("##dd_Scope", scopeList, 4, cb.presetScope);
            syncDropdown("##dd_Barrel", barrelList, 4, cb.presetBarrel);
            if (cb.useCustomSpeed) {
              g_gameState.xSpeed = cb.presetXSpeed;
              g_gameState.ySpeed = cb.presetYSpeed;
            }
            updateScaledPattern();
          }
        }
        customKeyWasDown[cb.keybind] = down;
      }
    }

    {
      static bool modeSwitchWasDown = false;
      if (g_gameState.modeSwitchKey != 0) {
        bool down = (GetAsyncKeyState(g_gameState.modeSwitchKey) & 0x8000) != 0;
        if (down && !modeSwitchWasDown) {
          g_gameState.scriptMode = (g_gameState.scriptMode == 0) ? 1 : 0;
        }
        modeSwitchWasDown = down;
      }
    }

    if (!g_menuVisible) {
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();
      ImGui::EndFrame();
      Sleep(10);
      continue;
    }

    g_menuAlpha = MR_Damp(g_menuAlpha, g_menuVisible ? 1.0f : 0.0f, 12.0f, dt);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_menuAlpha > 0.01f) {

      ImGuiIO &io = ImGui::GetIO();
      float winW = io.DisplaySize.x, winH = io.DisplaySize.y;
      float alpha = g_menuAlpha;
      bool input = g_menuVisible;

      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(ImVec2(winW, winH));
      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::Begin("##MainMenu", NULL,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse);
      {
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 winPos(0, 0);
        ImVec2 winSize(winW, winH);

        dl->AddRectFilled(winPos, winPos + winSize,
                          ImColor(menu_colors::background.x,
                                  menu_colors::background.y,
                                  menu_colors::background.z, 1.0f * alpha),
                          menu_colors::bg::rounding);

        if (g_gameState.orbsEnabled) {
          struct MenuOrb {
            float x, y, size, speed, drift, baseAlpha;
            float flickerOff, flickerSpd, pulseOff, pulseSpd;
            int depth;
          };
          static std::vector<MenuOrb> menuOrbs;
          static bool menuOrbsInit = false;
          if (!menuOrbsInit) {
            menuOrbsInit = true;
            menuOrbs.resize(20);
            for (auto &o : menuOrbs) {
              float rv = (float)rand() / RAND_MAX;
              o.depth = (rv < 0.5f) ? 0 : (rv < 0.85f ? 1 : 2);
              o.x = (float)(rand() % (int)winW);
              o.y = (float)(rand() % (int)winH);
              float sMin[] = {1,2,3}, sMax[] = {2,4,6};
              o.size = sMin[o.depth] + (sMax[o.depth]-sMin[o.depth]) * ((float)rand()/RAND_MAX);
              float spMin[] = {0.3f,0.6f,1.0f}, spMax[] = {0.6f,1.2f,2.0f};
              o.speed = spMin[o.depth] + (spMax[o.depth]-spMin[o.depth]) * ((float)rand()/RAND_MAX);
              float aMin[] = {40,80,150}, aMax[] = {80,150,255};
              o.baseAlpha = aMin[o.depth] + (aMax[o.depth]-aMin[o.depth]) * ((float)rand()/RAND_MAX);
              float dMin[] = {-0.05f,-0.15f,-0.25f}, dMax[] = {0.05f,0.15f,0.25f};
              o.drift = dMin[o.depth] + (dMax[o.depth]-dMin[o.depth]) * ((float)rand()/RAND_MAX);
              o.flickerOff = 6.28f * ((float)rand()/RAND_MAX);
              o.flickerSpd = 0.03f + 0.09f * ((float)rand()/RAND_MAX);
              o.pulseOff = 6.28f * ((float)rand()/RAND_MAX);
              o.pulseSpd = 0.02f + 0.04f * ((float)rand()/RAND_MAX);
            }
          }

          float dtScale = dt * 60.0f;
          for (auto &o : menuOrbs) {
            o.y -= o.speed * dtScale;
            o.x += o.drift * dtScale;
            o.flickerOff += o.flickerSpd * dtScale;
            o.pulseOff += o.pulseSpd * dtScale;
            if (o.y < -(o.size * 4)) {
              o.y = winH + 5.f + 45.f * ((float)rand()/RAND_MAX);
              o.x = (float)(rand() % (int)winW);
            }
            if (o.x < -o.size) o.x = winW + o.size;
            if (o.x > winW + o.size) o.x = -o.size;
          }
          ImVec4 ac = g_accentColor;
          dl->PushClipRect(winPos, winPos + winSize, true);
          for (auto &o : menuOrbs) {
            float hFactor = 1.0f;
            if (o.y > winH - 60.f)
              hFactor = fmaxf(0.f, (winH - o.y) / 60.f);
            else if (o.y < 80.f)
              hFactor = fmaxf(0.f, o.y / 80.f);
            float flicker = 0.85f + 0.15f * sinf(o.flickerOff);
            float depthFactor = 0.6f + o.depth * 0.2f;
            float orbAlpha = (o.baseAlpha / 255.f) * hFactor * flicker * depthFactor * alpha;
            if (orbAlpha < 0.005f) continue;
            float drawSize = o.size;
            if (o.depth == 2) drawSize *= (1.f + 0.15f * sinf(o.pulseOff));
            float ox = o.x, oy = o.y;
            float maxR = drawSize * 3.0f;
            const int rings = 10;
            for (int ri = rings; ri >= 1; ri--) {
              float rt = (float)ri / (float)rings;
              float ringR = maxR * rt;
              float ringA = orbAlpha * (1.f - rt * rt) * 0.65f;
              if (ringA < 0.003f) continue;
              dl->AddCircleFilled(ImVec2(ox, oy), ringR,
                                  ImColor(ac.x, ac.y, ac.z, ringA), 0);
            }
            dl->AddCircleFilled(ImVec2(ox, oy), drawSize * 0.4f,
                                ImColor(ac.x, ac.y, ac.z, orbAlpha * 0.85f), 0);
          }
          dl->PopClipRect();
        }

        ImVec2 minPos(winPos.x + winW - 58, winPos.y + 10);
        ImGui::SetCursorScreenPos(minPos);
        if (ImGui::InvisibleButton("##menu_minimize", ImVec2(20, 20)))
          ShowWindow(g_menuHwnd, SW_MINIMIZE);

        bool minHov = ImGui::IsItemHovered();
        static float minHoverAnim = 0.f;
        minHoverAnim += (minHov ? 1.f : -1.f) * dt * 8.f;
        minHoverAnim = minHoverAnim < 0.f ? 0.f : (minHoverAnim > 1.f ? 1.f : minHoverAnim);
        dl->AddLine(ImVec2(minPos.x + 4, minPos.y + 11),
                    ImVec2(minPos.x + 16, minPos.y + 11),
                    ImColor(1.f, 1.f, 1.f, alpha * (0.4f + minHoverAnim * 0.6f)),
                    1.5f);

        ImVec2 closePos(winPos.x + winW - 32, winPos.y + 10);
        ImGui::SetCursorScreenPos(closePos);
        if (ImGui::InvisibleButton("##menu_close", ImVec2(20, 20)))
          g_running = false;

        bool closeHov = ImGui::IsItemHovered();
        static float closeHoverAnim = 0.f;
        closeHoverAnim += (closeHov ? 1.f : -1.f) * dt * 8.f;
        closeHoverAnim = closeHoverAnim < 0.f ? 0.f : (closeHoverAnim > 1.f ? 1.f : closeHoverAnim);
        float xAlpha = 0.4f + closeHoverAnim * 0.6f;
        ImU32 xCol = ImColor(1.f, 1.f - closeHoverAnim * 0.7f,
                             1.f - closeHoverAnim * 0.7f, alpha * xAlpha);
        if (closeHoverAnim > 0.01f)
          dl->AddCircleFilled(
              ImVec2(closePos.x + 10, closePos.y + 10), 12.f,
              ImColor(1.f, 0.3f, 0.3f, alpha * closeHoverAnim * 0.3f));
        dl->AddLine(ImVec2(closePos.x + 4, closePos.y + 4),
                    ImVec2(closePos.x + 16, closePos.y + 16), xCol,
                    1.5f + closeHoverAnim * 0.5f);
        dl->AddLine(ImVec2(closePos.x + 16, closePos.y + 4),
                    ImVec2(closePos.x + 4, closePos.y + 16), xCol,
                    1.5f + closeHoverAnim * 0.5f);

        float sidebarW = 140.0f;

        dl->AddRectFilled(winPos, ImVec2(winPos.x + sidebarW, winPos.y + winH),
                          ImColor(menu_colors::layout.x, menu_colors::layout.y,
                                  menu_colors::layout.z, 1.0f * alpha),
                          menu_colors::bg::rounding,
                          ImDrawFlags_RoundCornersLeft);

        dl->AddLine(ImVec2(winPos.x + sidebarW, winPos.y),
                    ImVec2(winPos.x + sidebarW, winPos.y + winH),
                    ImColor(20, 20, 20, (int)(255 * alpha)), 1.0f);

        auto DrawSidebarHeader = [&](float y, const char *text) {
          MR_DrawText(dl, g_fontTiny, winPos + ImVec2(20, y),
                      ImColor(150, 150, 150, (int)(255 * alpha)), text);
        };

        auto DrawSidebarTab = [&](float &y, int tabIdx, const char *icon,
                                  const char *label) -> bool {
          bool isActive = (g_activeTab == tabIdx);
          ImVec2 pos = winPos + ImVec2(10, y);
          ImVec2 size(sidebarW - 20, 36);
          ImRect bb(pos, pos + size);

          bool hovered, held;
          bool pressed =
              ImGui::ButtonBehavior(bb, ImGui::GetID(label), &hovered, &held);

          if (isActive) {
            dl->AddRectFilled(pos, pos + size,
                              ImColor(g_accentColor.x * 0.4f,
                                      g_accentColor.y * 0.1f,
                                      g_accentColor.z * 0.1f, alpha * 0.5f),
                              8.0f);
          } else if (hovered) {
            dl->AddRectFilled(pos, pos + size,
                              ImColor(1.0f, 1.0f, 1.0f, alpha * 0.05f), 8.0f);
          }

          ImVec4 iconCol =
              isActive ? g_accentColor : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
          ImVec4 textCol = isActive ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                                    : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

          MR_DrawText(dl, g_fontHeader, pos + ImVec2(12, 10),
                      ImColor(iconCol.x, iconCol.y, iconCol.z, alpha), icon);
          MR_DrawText(dl, g_fontHeader, pos + ImVec2(40, 8),
                      ImColor(textCol.x, textCol.y, textCol.z, alpha), label);

          y += 42.0f;
          if (pressed && !isActive) {
            g_prevTab = g_activeTab;
            g_activeTab = tabIdx;
            g_tabAnimProgress = 0.0f;
          }
          return pressed;
        };

        float sy = 90.0f;
        if (g_logoTexture) {
          dl->AddImage((ImTextureID)g_logoTexture,
                       winPos + ImVec2(sidebarW * 0.5f - 24, 25),
                       winPos + ImVec2(sidebarW * 0.5f + 24, 25 + 48),
                       ImVec2(0, 0), ImVec2(1, 1),
                       ImColor(g_accentColor.x, g_accentColor.y,
                               g_accentColor.z, alpha));
        }

        DrawSidebarHeader(sy, "Recoil");
        sy += 20.0f;
        DrawSidebarTab(sy, 0, ICON_FA_CROSSHAIRS, "Recoil");
        DrawSidebarTab(sy, 1, ICON_FA_SLIDERS_H, "Tuning");
        sy += 10.0f;

        DrawSidebarHeader(sy, "System");
        sy += 20.0f;
        DrawSidebarTab(sy, 2, ICON_FA_COG, "Settings");
        DrawSidebarTab(sy, 3, ICON_FA_KEYBOARD, "Keybinds");

        g_tabAnimProgress = MR_Damp(g_tabAnimProgress, 1.0f, 10.0f, dt);
        g_textAnimProgress = MR_Damp(g_textAnimProgress, 1.0f, 8.0f, dt);

        float headerH = 60.0f;
        static int activeSubTabs[4] = {0, 0, 0, 0};

        auto DrawSubTab = [&](ImVec2 pos, int idx, const char *label) -> float {
          bool isActive = (activeSubTabs[g_activeTab] == idx);
          ImVec2 textSize = ImGui::CalcTextSize(label);
          ImVec2 size(textSize.x + 30.0f, 30.0f);
          ImRect bb(pos, pos + size);

          std::string lId = std::string("sub_") + label;
          bool hovered, held;
          bool pressed = ImGui::ButtonBehavior(bb, ImGui::GetID(lId.c_str()),
                                               &hovered, &held);

          if (isActive) {
            dl->AddRectFilled(pos, pos + size,
                              ImColor(menu_colors::accent_dim.x,
                                      menu_colors::accent_dim.y,
                                      menu_colors::accent_dim.z, alpha),
                              15.0f);
          } else if (hovered) {
            dl->AddRectFilled(pos, pos + size,
                              ImColor(menu_colors::lightchild.x,
                                      menu_colors::lightchild.y,
                                      menu_colors::lightchild.z, alpha),
                              15.0f);
          }

          ImVec4 tcol =
              isActive ? g_accentColor : (hovered ? g_textCol : g_textDisabled);
          ImVec2 textPos = pos + ImVec2((size.x - textSize.x) * 0.5f,
                                        (size.y - textSize.y) * 0.5f);
          MR_DrawText(dl, g_fontSmall, textPos,
                      ImColor(tcol.x, tcol.y, tcol.z, alpha), label);

          if (pressed)
            activeSubTabs[g_activeTab] = idx;
          return size.x + 10.0f;
        };

        ImVec2 subPos = winPos + ImVec2(sidebarW + 30.0f, 25.0f);

        if (g_activeTab == 0) {
          float nextX = DrawSubTab(subPos, 0, "Weapon");
          subPos.x += nextX;
        } else if (g_activeTab == 1) {
          float nextX = DrawSubTab(subPos, 0, "Sensitivity");
          subPos.x += nextX;
        } else if (g_activeTab == 2) {
          float nextX = DrawSubTab(subPos, 0, "Settings");
          subPos.x += nextX;
        } else if (g_activeTab == 3) {
          DrawSubTab(subPos, 0, "Custom Binds");
        }

        ImVec2 contentPos = winPos + ImVec2(sidebarW + 40, headerH + 20);
        float contentW = winSize.x - sidebarW - 80;
        float contentAlpha = alpha * g_tabAnimProgress;

        switch (g_activeTab) {
        case 0:
          RenderRecoilTab(dl, contentPos, contentW, contentAlpha, dt, input,
                          winPos, winSize, activeSubTabs[0]);
          break;
        case 1:
          RenderTuningTab(dl, contentPos, contentW, contentAlpha, dt, input,
                          winPos, winSize, activeSubTabs[1]);
          break;
        case 2:
          RenderSettingsTab(dl, contentPos, contentW, contentAlpha, dt, input,
                            winPos, winSize, activeSubTabs[2]);
          break;
        case 3:
          RenderKeybindsTab(dl, contentPos, contentW, contentAlpha, dt, input,
                            winPos, winSize, activeSubTabs[3]);
          break;
        }
      }
      ImGui::End();
      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor();
    }

    ImGui::EndFrame();
    ImGui::Render();
    float clear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    g_pd3dContext->OMSetRenderTargets(1, &g_rtv, NULL);
    g_pd3dContext->ClearRenderTargetView(g_rtv, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(0, 0);
  }
}

void CleanupMainMenu() {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
  if (g_rtv)
    g_rtv->Release();
  if (g_pSwapChain)
    g_pSwapChain->Release();
  if (g_pd3dContext)
    g_pd3dContext->Release();
  if (g_pd3dDevice)
    g_pd3dDevice->Release();
  DestroyWindow(g_menuHwnd);
}
