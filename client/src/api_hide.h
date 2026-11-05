#pragma once

#include <cstdint>
#include <windows.h>

namespace api_hide {

constexpr uint32_t ct_hash(const char *str) {
  uint32_t hash = 5381;
  while (*str) {
    hash = ((hash << 5) + hash) + (uint8_t)*str;
    str++;
  }
  return hash;
}

inline uint32_t rt_hash(const char *str) {
  uint32_t hash = 5381;
  while (*str) {
    hash = ((hash << 5) + hash) + (uint8_t)*str;
    str++;
  }
  return hash;
}

inline void *resolve_by_hash(HMODULE hMod, uint32_t targetHash) {
  if (!hMod)
    return nullptr;

  PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hMod;
  PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE *)hMod + dos->e_lfanew);
  DWORD exportRVA =
      nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
          .VirtualAddress;
  if (!exportRVA)
    return nullptr;

  PIMAGE_EXPORT_DIRECTORY exports =
      (PIMAGE_EXPORT_DIRECTORY)((BYTE *)hMod + exportRVA);
  DWORD *names = (DWORD *)((BYTE *)hMod + exports->AddressOfNames);
  WORD *ordinals = (WORD *)((BYTE *)hMod + exports->AddressOfNameOrdinals);
  DWORD *functions = (DWORD *)((BYTE *)hMod + exports->AddressOfFunctions);

  for (DWORD i = 0; i < exports->NumberOfNames; i++) {
    const char *name = (const char *)((BYTE *)hMod + names[i]);
    if (rt_hash(name) == targetHash) {
      return (void *)((BYTE *)hMod + functions[ordinals[i]]);
    }
  }
  return nullptr;
}

constexpr uint32_t HASH_KERNEL32 = ct_hash("kernel32.dll");
constexpr uint32_t HASH_NTDLL = ct_hash("ntdll.dll");
constexpr uint32_t HASH_USER32 = ct_hash("user32.dll");

inline HMODULE get_module_peb(uint32_t nameHash) {
  if (nameHash == HASH_KERNEL32)
    return GetModuleHandleA("kernel32.dll");
  else if (nameHash == HASH_NTDLL)
    return GetModuleHandleA("ntdll.dll");
  else if (nameHash == HASH_USER32)
    return GetModuleHandleA("user32.dll");
  return nullptr;
}

constexpr uint32_t HASH_IsDebuggerPresent = ct_hash("IsDebuggerPresent");
constexpr uint32_t HASH_CheckRemoteDebuggerPresent =
    ct_hash("CheckRemoteDebuggerPresent");
constexpr uint32_t HASH_NtQueryInformationProcess =
    ct_hash("NtQueryInformationProcess");
constexpr uint32_t HASH_ExitProcess = ct_hash("ExitProcess");
constexpr uint32_t HASH_GetThreadContext = ct_hash("GetThreadContext");
constexpr uint32_t HASH_VirtualProtect = ct_hash("VirtualProtect");
constexpr uint32_t HASH_GetCurrentThread = ct_hash("GetCurrentThread");
constexpr uint32_t HASH_GetCurrentProcess = ct_hash("GetCurrentProcess");
constexpr uint32_t HASH_CloseHandle = ct_hash("CloseHandle");

typedef BOOL(WINAPI *fn_IsDebuggerPresent)();
typedef BOOL(WINAPI *fn_CheckRemoteDebuggerPresent)(HANDLE, PBOOL);
typedef LONG(NTAPI *fn_NtQueryInformationProcess)(HANDLE, ULONG, PVOID, ULONG,
                                                  PULONG);
typedef void(WINAPI *fn_ExitProcess)(UINT);
typedef BOOL(WINAPI *fn_GetThreadContext)(HANDLE, LPCONTEXT);
typedef HANDLE(WINAPI *fn_GetCurrentThread)();
typedef HANDLE(WINAPI *fn_GetCurrentProcess)();
typedef BOOL(WINAPI *fn_CloseHandle)(HANDLE);

struct HiddenAPIs {
  fn_IsDebuggerPresent pIsDebuggerPresent = nullptr;
  fn_CheckRemoteDebuggerPresent pCheckRemoteDebuggerPresent = nullptr;
  fn_NtQueryInformationProcess pNtQueryInformationProcess = nullptr;
  fn_ExitProcess pExitProcess = nullptr;
  fn_GetThreadContext pGetThreadContext = nullptr;
  fn_GetCurrentThread pGetCurrentThread = nullptr;
  fn_GetCurrentProcess pGetCurrentProcess = nullptr;
  fn_CloseHandle pCloseHandle = nullptr;
  bool resolved = false;

  void resolve() {
    if (resolved)
      return;
    HMODULE k32 = get_module_peb(HASH_KERNEL32);
    HMODULE ntdll = get_module_peb(HASH_NTDLL);

    if (k32) {
      pIsDebuggerPresent =
          (fn_IsDebuggerPresent)resolve_by_hash(k32, HASH_IsDebuggerPresent);
      pCheckRemoteDebuggerPresent =
          (fn_CheckRemoteDebuggerPresent)resolve_by_hash(
              k32, HASH_CheckRemoteDebuggerPresent);
      pExitProcess = (fn_ExitProcess)resolve_by_hash(k32, HASH_ExitProcess);
      pGetCurrentThread =
          (fn_GetCurrentThread)resolve_by_hash(k32, HASH_GetCurrentThread);
      pGetCurrentProcess =
          (fn_GetCurrentProcess)resolve_by_hash(k32, HASH_GetCurrentProcess);
      pGetThreadContext =
          (fn_GetThreadContext)resolve_by_hash(k32, HASH_GetThreadContext);
      pCloseHandle = (fn_CloseHandle)resolve_by_hash(k32, HASH_CloseHandle);
    }
    if (ntdll) {
      pNtQueryInformationProcess =
          (fn_NtQueryInformationProcess)resolve_by_hash(
              ntdll, HASH_NtQueryInformationProcess);
    }
    resolved = true;
  }
};

inline HiddenAPIs &getAPIs() {
  static HiddenAPIs apis;
  if (!apis.resolved)
    apis.resolve();
  return apis;
}

inline void stealth_exit() {
  auto &api = getAPIs();
  if (api.pExitProcess)
    api.pExitProcess(0);

  ExitProcess(0);
}

}
