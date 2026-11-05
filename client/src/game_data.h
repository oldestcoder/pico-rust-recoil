#pragma once
#include <Windows.h>
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class PrecisionTimer {
public:
  PrecisionTimer() { QueryPerformanceFrequency(&freq); }

  double getTime() const {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
  }

  void accurateSleep(double ms) const {
    if (ms <= 0)
      return;
    double target = getTime() + ms / 1000.0;
    while (getTime() < target) {
    }
  }

  void spinWait(double targetTime) const {
    while (getTime() < targetTime) {
    }
  }

private:
  LARGE_INTEGER freq;
};

struct WeaponData {
  std::vector<std::pair<float, float>> pattern;
  float delay;
  bool autoFire;
};

struct CustomKeybind {
  std::string id;
  std::string actionType = "option";
  std::string actionValue = "Master Toggle";
  int keybind = 0;
  std::string type = "toggle";
  bool enabled = true;

  std::string presetWeapon = "AK-47";
  std::string presetScope = "None";
  std::string presetBarrel = "None";
  bool useCustomSpeed = false;
  float presetXSpeed = 1.0f;
  float presetYSpeed = 1.0f;
};

struct GameState {
  std::string currentWeapon = "AK-47";
  std::string currentScope = "None";
  std::string currentBarrel = "None";
  float sensitivity = 0.28f;
  float adsSensitivity = 0.76f;
  float fov = 90.0f;
  bool recoilEnabled = false;
  int smoothingSteps = 500;
  bool movementDetectionEnabled = true;
  bool hipfireEnabled = false;
  float xSpeed = 1.0f;
  float ySpeed = 1.0f;
  bool autoFlickEnabled = false;
  int autoFlickValue = 600;

  int scriptMode = 0;

  int themeIndex = 1;

  bool orbsEnabled = true;

  int toggleKey = 0;
  int hideMenuKey = 0;
  int modeSwitchKey = 0;

  std::map<std::string, int> weaponKeybinds;
  std::map<std::string, int> scopeKeybinds;
  std::map<std::string, int> barrelKeybinds;

  int crouchKey = VK_LCONTROL;
  int moveLeftKey = 0x41;
  int moveDownKey = 0x53;
  int moveRightKey = 0x44;
  int moveUpKey = 0x57;

  int inputMethod = 0;

  std::vector<CustomKeybind> customKeybinds;
};

extern PrecisionTimer g_timer;
extern GameState g_gameState;

extern std::atomic<bool> g_lmbPressed;
extern std::atomic<bool> g_rmbPressed;
extern std::mutex g_buttonLock;

extern std::mutex g_patternLock;
extern std::vector<std::pair<float, float>> g_scaledPattern;
extern float g_scaledDelayMs;

extern std::map<std::string, WeaponData> g_weaponData;
extern std::map<std::string, float> g_scopes;
extern std::map<std::string, float> g_barrels;
extern std::map<std::string, float> g_weaponSpeedFactors;
extern std::map<std::string, float> g_hipfireMultipliers;

void initWeaponData();
float getMovementMultiplier();
void updateScaledPattern();
