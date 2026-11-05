#include "mouse_input.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace {

class SerialMouseBridge final {
public:
  ~SerialMouseBridge() { disconnect(); }

  bool connect(const std::string &portName) {
    disconnect();

    const std::string devicePath = normalizePortName(portName);
    HANDLE handle = CreateFileA(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE,
                                0, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
      return false;

    if (!configure(handle)) {
      CloseHandle(handle);
      return false;
    }

    {
      std::lock_guard<std::mutex> guard(writeMutex_);
      handle_ = handle;
    }

    stopping_.store(false, std::memory_order_release);
    connected_.store(true, std::memory_order_release);
    reader_ = std::thread(&SerialMouseBridge::readerLoop, this);
    return true;
  }

  void disconnect() {
    stopping_.store(true, std::memory_order_release);
    if (reader_.joinable())
      reader_.join();

    std::lock_guard<std::mutex> guard(writeMutex_);
    connected_.store(false, std::memory_order_release);
    if (handle_ != INVALID_HANDLE_VALUE) {
      PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);
      CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
  }

  bool isConnected() const noexcept {
    return connected_.load(std::memory_order_acquire);
  }

  void moveRelative(int dx, int dy) {
    if ((dx == 0 && dy == 0) || !isConnected())
      return;

    // Firmware accepts signed 8-bit movement values: -127..127.
    // Split large deltas into valid commands while preserving their sum.
    while ((dx != 0 || dy != 0) && isConnected()) {
      const int stepX = std::clamp(dx, -127, 127);
      const int stepY = std::clamp(dy, -127, 127);
      if (!sendMove(stepX, stepY))
        return;
      dx -= stepX;
      dy -= stepY;
    }
  }

private:
  static std::string normalizePortName(std::string port) {
    while (!port.empty() && (port.back() == '\r' || port.back() == '\n' ||
                             port.back() == ' ' || port.back() == '\t'))
      port.pop_back();
    const auto first = port.find_first_not_of(" \t");
    if (first != std::string::npos && first > 0)
      port.erase(0, first);

    if (port.rfind("\\\\.\\", 0) == 0)
      return port;
    if (port.size() >= 3 && port[0] == 'C' && port[1] == 'O' && port[2] == 'M')
      return "\\\\.\\" + port;
    return port;
  }

  static bool configure(HANDLE handle) {
    if (!SetupComm(handle, 4096, 4096))
      return false;

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle, &dcb))
      return false;

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError = FALSE;
    if (!SetCommState(handle, &dcb))
      return false;

    COMMTIMEOUTS timeouts{};
    // Reader polling is non-blocking; writes should normally fit into the
    // driver buffer immediately and must not stall the recoil timing loop.
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 5;
    if (!SetCommTimeouts(handle, &timeouts))
      return false;

    PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
  }

  bool sendMove(int dx, int dy) {
    char command[32];
    const int length = std::snprintf(command, sizeof(command), "M %d %d\n", dx, dy);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(command))
      return false;

    std::lock_guard<std::mutex> guard(writeMutex_);
    if (!connected_.load(std::memory_order_acquire) ||
        handle_ == INVALID_HANDLE_VALUE)
      return false;

    size_t offset = 0;
    while (offset < static_cast<size_t>(length)) {
      DWORD written = 0;
      const BOOL ok = WriteFile(handle_, command + offset,
                                static_cast<DWORD>(length - offset), &written,
                                nullptr);
      if (!ok || written == 0) {
        connected_.store(false, std::memory_order_release);
        return false;
      }
      offset += written;
    }
    return true;
  }

  void readerLoop() {
    char buffer[128];
    while (!stopping_.load(std::memory_order_acquire)) {
      HANDLE handle = handle_;
      if (handle == INVALID_HANDLE_VALUE)
        break;

      DWORD bytesRead = 0;
      if (!ReadFile(handle, buffer, sizeof(buffer), &bytesRead, nullptr)) {
        if (!stopping_.load(std::memory_order_acquire))
          connected_.store(false, std::memory_order_release);
        break;
      }

      // The firmware replies `OK\n` per accepted command. We intentionally
      // drain and discard replies here so the serial RX buffer never fills;
      // movement timing does not wait for an ACK round-trip.
      if (bytesRead == 0)
        Sleep(1);
    }
  }

  HANDLE handle_ = INVALID_HANDLE_VALUE;
  std::atomic<bool> connected_{false};
  std::atomic<bool> stopping_{false};
  std::mutex writeMutex_;
  std::thread reader_;
};

SerialMouseBridge g_bridge;

std::string readConfiguredPort() {
  char exePath[MAX_PATH]{};
  const DWORD size = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  if (size == 0 || size >= MAX_PATH)
    return "COM5";

  std::string path(exePath, size);
  const size_t slash = path.find_last_of("\\/");
  path.resize(slash == std::string::npos ? 0 : slash + 1);
  path += "bridge_port.txt";

  std::ifstream file(path);
  std::string port;
  if (file && std::getline(file, port)) {
    const auto first = port.find_first_not_of(" \t\r\n");
    if (first != std::string::npos) {
      const auto last = port.find_last_not_of(" \t\r\n");
      return port.substr(first, last - first + 1);
    }
  }
  return "COM5";
}

} // namespace

namespace MouseInput {

bool connect() { return g_bridge.connect(readConfiguredPort()); }

void disconnect() { g_bridge.disconnect(); }

bool isConnected() { return g_bridge.isConnected(); }

void moveRelative(int dx, int dy) { g_bridge.moveRelative(dx, dy); }

} // namespace MouseInput
