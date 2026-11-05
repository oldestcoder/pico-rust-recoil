#include "security.h"
#include "api_hide.h"
#include <Windows.h>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

static bool checkDebugger() {
  auto &api = api_hide::getAPIs();

  if (api.pIsDebuggerPresent && api.pIsDebuggerPresent())
    return true;

  if (api.pCheckRemoteDebuggerPresent) {
    BOOL isRemote = FALSE;
    api.pCheckRemoteDebuggerPresent(
        api.pGetCurrentProcess ? api.pGetCurrentProcess()
                               : GetCurrentProcess(),
        &isRemote);
    if (isRemote)
      return true;
  }

  if (api.pNtQueryInformationProcess) {
    HANDLE debugPort = NULL;
    ULONG ret = 0;
    LONG status = api.pNtQueryInformationProcess(
        GetCurrentProcess(), 7,
        &debugPort, sizeof(debugPort), &ret);
    if (status == 0 && debugPort != NULL)
      return true;
  }

  return false;
}

static bool checkHardwareBreakpoints() {
  auto &api = api_hide::getAPIs();
  if (!api.pGetThreadContext || !api.pGetCurrentThread)
    return false;

  CONTEXT ctx = {0};
  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
  if (api.pGetThreadContext(api.pGetCurrentThread(), &ctx)) {
    if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3)
      return true;
  }
  return false;
}

static bool checkTimingAnomaly() {

  LARGE_INTEGER freq, start, end;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&start);

  volatile int x = 0;
  for (int i = 0; i < 100; i++)
    x += i;

  QueryPerformanceCounter(&end);
  double elapsed_ms =
      (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)freq.QuadPart;

  return elapsed_ms > 200.0;
}

static bool checkBlacklistedProcesses() {

  static const std::vector<std::pair<std::string, std::string>> windowChecks = {
      {"", "x64dbg"}, {"", "x32dbg"},           {"Ollydbg", ""},
      {"WinDbgFrameClass", ""},
  };

  for (const auto &[className, windowName] : windowChecks) {
    const char *cls = className.empty() ? NULL : className.c_str();
    const char *wnd = windowName.empty() ? NULL : windowName.c_str();
    if (FindWindowA(cls, wnd) != NULL)
      return true;
  }

  static const std::vector<std::string> badTitles = {
      "cheat engine",
      "process hacker",
      "x64dbg",
      "x32dbg",
      "ollydbg",
      "ida: ",
      "ida pro",
      "ghidra:",
      "dnspy",
      "de4dot",
      "megadumper",
      "scylla",
      "http debugger pro",
  };

  struct EnumData {
    const std::vector<std::string> *titles;
    bool found;
  };

  EnumData data{&badTitles, false};

  EnumWindows(
      [](HWND hwnd, LPARAM lParam) -> BOOL {
        auto *d = reinterpret_cast<EnumData *>(lParam);
        char title[256];
        if (GetWindowTextA(hwnd, title, sizeof(title)) > 0) {
          std::string t(title);
          for (auto &c : t)
            c = (char)tolower((unsigned char)c);
          for (const auto &bad : *d->titles) {
            if (t.find(bad) != std::string::npos) {
              d->found = true;
              return FALSE;
            }
          }
        }
        return TRUE;
      },
      (LPARAM)&data);

  return data.found;
}

static DWORD g_textCRC = 0;
static void *g_textBase = nullptr;
static size_t g_textSize = 0;

static DWORD computeCRC32(const void *data, size_t len) {
  const BYTE *p = (const BYTE *)data;
  DWORD crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= p[i];
    for (int j = 0; j < 8; j++) {
      DWORD mask = (crc & 1) ? 0xFFFFFFFF : 0;
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
  }
  return ~crc;
}

static void initTextIntegrity() {
  HMODULE hMod = GetModuleHandleA(NULL);
  if (!hMod)
    return;
  PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hMod;
  PIMAGE_NT_HEADERS nt =
      (PIMAGE_NT_HEADERS)((BYTE *)hMod + dos->e_lfanew);
  PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
  for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
    if (memcmp(sec[i].Name, ".text", 5) == 0) {
      g_textBase = (void *)((BYTE *)hMod + sec[i].VirtualAddress);
      g_textSize = sec[i].Misc.VirtualSize;
      g_textCRC = computeCRC32(g_textBase, g_textSize);
      return;
    }
  }
}

static bool checkTextIntegrity() {
  if (!g_textBase || g_textSize == 0)
    return false;
  return computeCRC32(g_textBase, g_textSize) != g_textCRC;
}

static void erasePEHeaders() {
  HMODULE hMod = GetModuleHandleA(NULL);
  if (!hMod)
    return;
  PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hMod;
  DWORD headerSize =
      ((PIMAGE_NT_HEADERS)((BYTE *)hMod + dos->e_lfanew))
          ->OptionalHeader.SizeOfHeaders;
  DWORD oldProt;
  VirtualProtect(hMod, headerSize, PAGE_READWRITE, &oldProt);
  SecureZeroMemory(hMod, headerSize);
  VirtualProtect(hMod, headerSize, oldProt, &oldProt);
}

static std::atomic<bool> g_securityRunning{false};
static std::atomic<int> g_violationCount{0};

static void securityMonitorLoop() {
  int checkIndex = 0;
  while (g_securityRunning) {
    bool violation = false;

    switch (checkIndex % 3) {
    case 0:
      violation = checkDebugger();
      break;
    case 1:
      violation = checkBlacklistedProcesses();
      break;
    case 2:
      violation = checkHardwareBreakpoints();
      break;
    }
    checkIndex++;

    if (violation) {
      g_violationCount++;
      if (g_violationCount >= 3) {
        auto &api = api_hide::getAPIs();
        if (api.pExitProcess)
          api.pExitProcess(0);
        ExitProcess(0);
      }
    } else {
      g_violationCount = 0;
    }

    Sleep(5000 + (rand() % 7000));
  }
}

std::pair<bool, std::string> startSecurityCheck() {
  if (checkDebugger())
    return {false, ""};
  if (checkBlacklistedProcesses())
    return {false, ""};
  if (checkHardwareBreakpoints())
    return {false, ""};

  g_securityRunning = true;
  std::thread monitorThread(securityMonitorLoop);
  monitorThread.detach();

  return {true, ""};
}
