#include "recoil_legit.h"
#include "mouse_input.h"
#include <cmath>
#include <cstdio>
#include <random>

LegitRecoil g_legitEngine;

static void setThreadHighPriority() {
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
}

void LegitRecoil::start() {
  std::lock_guard<std::mutex> lk(lock);
  if (!active) {
    active = true;
    recoilThread = std::thread(&LegitRecoil::loop, this);
    recoilThread.detach();
    printf("[LEGIT] Recoil Started\n");
  }
}

void LegitRecoil::stop() {
  std::lock_guard<std::mutex> lk(lock);
  if (active) {
    active = false;
    printf("[LEGIT] Recoil Stopped\n");
  }
}

void LegitRecoil::loop() {
  setThreadHighPriority();

  int index = 0;
  bool wasActive = false;
  double shotStartTime = 0.0;
  double accX = 0.0, accY = 0.0;

  std::vector<std::pair<float, float>> basePattern;
  float cachedDelayMs = 0.0f;
  bool cachedHipfireState = false;
  float cachedWeaponSpeed = 1.0f;
  float cachedScopeMult = 1.0f;
  float cachedBarrelMult = 1.0f;
  float cachedSensFovMult = 1.0f;
  float currentMovementMult = 1.0f;

  bool initialDelayCompensated = false;
  float timeDebtMs = 0.0f;
  bool lmbWasPressed = false;

  double semiLastShotTime = 0.0;
  bool semiWaitingForRelease = false;

  std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<float> randDist(40.0f, 60.0f);

  while (active) {
    bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool rmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    bool isActive = g_gameState.hipfireEnabled
                        ? (lmb && g_gameState.recoilEnabled)
                        : (lmb && rmb && g_gameState.recoilEnabled);
    bool shouldReset = g_gameState.hipfireEnabled ? !lmb : !(lmb && rmb);

    if (wasActive && shouldReset) {
      index = 0;
      shotStartTime = 0.0;
      accX = accY = 0.0;
      basePattern.clear();
      cachedDelayMs = 0.0f;
      cachedHipfireState = false;
      cachedWeaponSpeed = 1.0f;
      cachedScopeMult = cachedBarrelMult = cachedSensFovMult = 1.0f;
      currentMovementMult = 1.0f;
      initialDelayCompensated = false;
      timeDebtMs = 0.0f;
      lmbWasPressed = false;
    }
    wasActive = isActive;

    if (!isActive) {
      lmbWasPressed = false;
      semiWaitingForRelease = false;
      index = 0;
      accX = accY = 0.0;
      basePattern.clear();
      Sleep(1);
      continue;
    }

    bool isActuallyHipfiring = g_gameState.hipfireEnabled && lmb && !rmb;

    if (basePattern.empty()) {
      cachedHipfireState = isActuallyHipfiring;
      auto wsIt = g_weaponSpeedFactors.find(g_gameState.currentWeapon);
      cachedWeaponSpeed =
          (wsIt != g_weaponSpeedFactors.end()) ? wsIt->second : 1.0f;

      auto &wdata = g_weaponData[g_gameState.currentWeapon];
      basePattern = wdata.pattern;
      cachedDelayMs = wdata.delay;

      auto scopeIt = g_scopes.find(
          isActuallyHipfiring ? "None" : g_gameState.currentScope);
      cachedScopeMult = (scopeIt != g_scopes.end()) ? scopeIt->second : 1.0f;
      auto barrelIt = g_barrels.find(
          isActuallyHipfiring ? "None" : g_gameState.currentBarrel);
      cachedBarrelMult =
          (barrelIt != g_barrels.end()) ? barrelIt->second : 1.0f;

      cachedSensFovMult =
          -0.03f * (g_gameState.sensitivity * g_gameState.adsSensitivity) *
          3.0f * (g_gameState.fov / 100.0f);
      if (fabs(cachedSensFovMult) < 0.0001f)
        cachedSensFovMult = -0.02f;

      currentMovementMult =
          g_gameState.movementDetectionEnabled ? getMovementMultiplier() : 1.0f;

      if (!initialDelayCompensated) {
        float reactionDelay = randDist(rng);
        g_timer.accurateSleep(reactionDelay);
        timeDebtMs = reactionDelay;
        initialDelayCompensated = true;
      }

      shotStartTime = g_timer.getTime();
    }

    if (basePattern.empty()) {
      Sleep(1);
      continue;
    }

    // Always sample this value: getMovementMultiplier() returns 1.0 when
    // movement detection is disabled. Skipping the update here left the old
    // crouch/movement multiplier active until the trigger was released.
    const float newMult = getMovementMultiplier();
    if (fabs(newMult - currentMovementMult) > 0.01f)
      currentMovementMult = newMult;

    auto &wdata = g_weaponData[g_gameState.currentWeapon];
    bool isAuto = wdata.autoFire;

    if (!isAuto) {
      if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        lmbWasPressed = false;
        semiWaitingForRelease = false;
        index = 0;
        basePattern.clear();
        Sleep(1);
        continue;
      }

      if (semiWaitingForRelease) {
        Sleep(1);
        continue;
      }

      if (semiLastShotTime > 0.0) {
        auto &wd = g_weaponData[g_gameState.currentWeapon];
        double cooldownSec = wd.delay / 1000.0;
        if (g_timer.getTime() - semiLastShotTime < cooldownSec) {
          Sleep(1);
          continue;
        }
      }
      if (lmbWasPressed) {
        Sleep(1);
        continue;
      }
      lmbWasPressed = true;
    }

    int safeIndex = index % (int)basePattern.size();
    float baseX = basePattern[safeIndex].first;
    float baseY = basePattern[safeIndex].second;

    float moveMultiplier = cachedSensFovMult * currentMovementMult;
    float targetX =
        (baseX / moveMultiplier) * cachedScopeMult * cachedBarrelMult;
    float targetY =
        (baseY / moveMultiplier) * cachedScopeMult * cachedBarrelMult;

    if (cachedHipfireState) {
      auto hipIt = g_hipfireMultipliers.find(g_gameState.currentWeapon);
      float mult = (hipIt != g_hipfireMultipliers.end()) ? hipIt->second : 6.0f;
      targetX *= mult;
      targetY *= mult;
    }

    targetX *= g_gameState.xSpeed;
    targetY *= g_gameState.ySpeed;

    float baseDelayMs = cachedDelayMs / cachedWeaponSpeed;
    float actualDelayMs = baseDelayMs;
    if (timeDebtMs > 0) {
      float compensation =
          (timeDebtMs < baseDelayMs * 0.3f) ? timeDebtMs : baseDelayMs * 0.3f;
      actualDelayMs = baseDelayMs - compensation;
      timeDebtMs -= compensation;
    }

    int maxSafeSteps = (int)(actualDelayMs / 2);
    if (maxSafeSteps < 10)
      maxSafeSteps = 10;
    int steps = g_gameState.smoothingSteps;
    if (steps > maxSafeSteps)
      steps = maxSafeSteps;
    if (steps < 1)
      steps = 1;

    double timePerStep = (actualDelayMs / 1000.0) / steps;
    double dxPerStep = targetX / steps;
    double dyPerStep = targetY / steps;
    double shotTargetTime = shotStartTime + (actualDelayMs / 1000.0);

    for (int i = 0; i < steps; i++) {
      if (!active || !g_gameState.recoilEnabled)
        break;
      bool checkLmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
      bool checkRmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
      if ((g_gameState.hipfireEnabled && !checkLmb) ||
          (!g_gameState.hipfireEnabled && !(checkLmb && checkRmb)))
        break;

      double stepTargetTime = shotStartTime + ((i + 1) * timePerStep);

      accX += dxPerStep;
      accY += dyPerStep;

      int moveX = (int)round(accX);
      int moveY = (int)round(accY);

      if (moveX != 0 || moveY != 0) {
        accX -= moveX;
        accY -= moveY;
        MouseInput::moveRelative(moveX, moveY);
      }

      while (g_timer.getTime() < stepTargetTime) {
      }
    }

    while (g_timer.getTime() < shotTargetTime) {
    }

    shotStartTime = shotTargetTime;

    if (isAuto) {
      index++;
      if (index % 5 == 0) {
        if (fabs(accX) >= 0.5 || fabs(accY) >= 0.5) {
          int fx = (int)round(accX);
          int fy = (int)round(accY);
          if (fx != 0 || fy != 0)
            MouseInput::moveRelative(fx, fy);
          accX = accY = 0.0;
        }
      }
    } else {
      semiLastShotTime = g_timer.getTime();
      semiWaitingForRelease = true;
      index = 0;
      shotStartTime = 0.0;
      accX = accY = 0.0;
      basePattern.clear();
    }
  }
}
