#include "recoil_blatant.h"
#include "mouse_input.h"
#include <cmath>
#include <cstdio>

BlatantRecoil g_blatantEngine;

static void setThreadHighPriority() {
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
}

void BlatantRecoil::start() {
  std::lock_guard<std::mutex> lk(lock);
  if (!active) {
    active = true;
    recoilThread = std::thread(&BlatantRecoil::loop, this);
    recoilThread.detach();
    printf("[BLATANT] Recoil Started\n");
  }
}

void BlatantRecoil::stop() {
  std::lock_guard<std::mutex> lk(lock);
  if (active) {
    active = false;
    printf("[BLATANT] Recoil Stopped\n");
  }
}

void BlatantRecoil::loop() {
  setThreadHighPriority();

  int index = 0;
  double shotStartTime = 0.0;
  double accX = 0.0, accY = 0.0;
  std::vector<std::pair<float, float>> cachedPattern;
  float cachedDelayMs = 0.0f;
  bool burstActive = false;
  bool localLmbWasPressed = false;

  float currentMovementMult = 1.0f;
  int shotCount = 0;

  float timingErrorAccumulator = 0.0f;
  double lastShotActualTime = 0.0;

  double semiLastShotTime = 0.0;
  bool semiWaitingForRelease = false;

  auto resetBurstState = [&]() {
    index = 0;
    shotStartTime = 0.0;
    accX = accY = 0.0;
    localLmbWasPressed = false;
    cachedPattern.clear();
    cachedDelayMs = 0.0f;
    burstActive = false;
    shotCount = 0;
    currentMovementMult = 1.0f;
    timingErrorAccumulator = 0.0f;
    lastShotActualTime = 0.0;
  };

  auto recalcPatternWithMult = [&](float movementMult)
      -> std::pair<std::vector<std::pair<float, float>>, float> {
    auto &wdata = g_weaponData[g_gameState.currentWeapon];
    const auto &pattern = wdata.pattern;
    float delayMs = wdata.delay;

    bool isActuallyHipfiring = g_gameState.hipfireEnabled &&
                               (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 &&
                               (GetAsyncKeyState(VK_RBUTTON) & 0x8000) == 0;

    float scopeMult = 1.0f, barrelMult = 1.0f;
    if (!isActuallyHipfiring) {
      auto sIt = g_scopes.find(g_gameState.currentScope);
      scopeMult = (sIt != g_scopes.end()) ? sIt->second : 1.0f;
      auto bIt = g_barrels.find(g_gameState.currentBarrel);
      barrelMult = (bIt != g_barrels.end()) ? bIt->second : 1.0f;
    }

    float moveMultiplier =
        -0.03f *
        ((g_gameState.sensitivity * g_gameState.adsSensitivity) *
         movementMult) *
        3.0f * (g_gameState.fov / 100.0f);
    if (fabs(moveMultiplier) < 0.0001f)
      moveMultiplier = -0.02f;

    std::vector<std::pair<float, float>> scaled;
    scaled.reserve(pattern.size());
    for (const auto &[x, y] : pattern) {
      float px = (x / moveMultiplier) * scopeMult * barrelMult;
      float py = (y / moveMultiplier) * scopeMult * barrelMult;
      scaled.push_back({px, py});
    }
    return {scaled, delayMs};
  };

  while (active) {
    bool currentLmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool currentRmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    bool isActive =
        g_gameState.hipfireEnabled
            ? (currentLmb && g_gameState.recoilEnabled)
            : (currentLmb && currentRmb && g_gameState.recoilEnabled);

    if (!isActive) {
      resetBurstState();
      Sleep(1);
      continue;
    }

    bool isActuallyHipfiring =
        g_gameState.hipfireEnabled && currentLmb && !currentRmb;

    float newMovementMult = getMovementMultiplier();

    if (!burstActive) {
      auto [pat, del] = recalcPatternWithMult(newMovementMult);
      cachedPattern = std::move(pat);
      cachedDelayMs = del;
      currentMovementMult = newMovementMult;

      shotStartTime = g_timer.getTime();
      lastShotActualTime = shotStartTime;
      burstActive = true;
      timingErrorAccumulator = 0.0f;
    } else {
      if (fabs(newMovementMult - currentMovementMult) > 0.01f) {
        auto [pat, del] = recalcPatternWithMult(newMovementMult);
        cachedPattern = std::move(pat);
        cachedDelayMs = del;
        currentMovementMult = newMovementMult;
      }
    }

    if (cachedPattern.empty()) {
      Sleep(1);
      continue;
    }

    auto &wdata = g_weaponData[g_gameState.currentWeapon];
    bool isAuto = wdata.autoFire;

    if (!isAuto) {
      if (!currentLmb) {
        resetBurstState();
        semiWaitingForRelease = false;
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
      if (localLmbWasPressed) {
        Sleep(1);
        continue;
      }
      localLmbWasPressed = true;
    }

    float targetX, targetY;
    if (index < (int)cachedPattern.size()) {
      targetX = cachedPattern[index].first;
      targetY = cachedPattern[index].second;
    } else {
      int safeIdx = index % (int)cachedPattern.size();
      targetX = cachedPattern[safeIdx].first;
      targetY = cachedPattern[safeIdx].second;
    }

    if (isActuallyHipfiring) {
      auto hipIt = g_hipfireMultipliers.find(g_gameState.currentWeapon);
      float mult = (hipIt != g_hipfireMultipliers.end()) ? hipIt->second : 6.0f;
      targetX *= mult;
      targetY *= mult;
    }

    if (shotStartTime == 0.0)
      shotStartTime = g_timer.getTime();

    auto wsIt = g_weaponSpeedFactors.find(g_gameState.currentWeapon);
    float weaponSpeed =
        (wsIt != g_weaponSpeedFactors.end()) ? wsIt->second : 1.0f;
    float actualDelayMs = cachedDelayMs / weaponSpeed;
    double targetDelaySeconds = actualDelayMs / 1000.0;

    double correctedDelay;
    if (shotCount < 3) {
      correctedDelay = targetDelaySeconds;
    } else {
      correctedDelay = targetDelaySeconds - (timingErrorAccumulator * 0.3);
      double minD = targetDelaySeconds * 0.9;
      double maxD = targetDelaySeconds * 1.1;
      if (correctedDelay < minD)
        correctedDelay = minD;
      if (correctedDelay > maxD)
        correctedDelay = maxD;
    }

    int steps = 25;
    double timePerStep = correctedDelay / steps;

    float shotTargetX = targetX * g_gameState.xSpeed;
    float shotTargetY = targetY * g_gameState.ySpeed;

    double shotTargetTime = shotStartTime + correctedDelay;

    accX += shotTargetX;
    accY += shotTargetY;

    int totalPixelsX = (accX >= 0) ? (int)(accX + 0.5) : (int)(accX - 0.5);
    int totalPixelsY = (accY >= 0) ? (int)(accY + 0.5) : (int)(accY - 0.5);

    accX -= totalPixelsX;
    accY -= totalPixelsY;

    int pixelsMovedX = 0, pixelsMovedY = 0;

    bool shotInterrupted = false;
    for (int i = 0; i < steps; i++) {
      const bool stepLmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
      const bool stepRmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
      const bool stepInputActive = g_gameState.hipfireEnabled
                                       ? stepLmb
                                       : (stepLmb && stepRmb);
      if (!active || !g_gameState.recoilEnabled || !stepInputActive) {
        shotInterrupted = true;
        break;
      }

      double stepTargetTime = shotStartTime + ((i + 1) * timePerStep);

      const float progress = (float)(i + 1) / steps;
      // lround is symmetric for positive and negative coordinates; adding
      // +0.5 before an integer cast biased negative horizontal recoil steps.
      const int targetByStepX = static_cast<int>(std::lround(totalPixelsX * progress));
      const int targetByStepY = static_cast<int>(std::lround(totalPixelsY * progress));

      const int moveX = targetByStepX - pixelsMovedX;
      const int moveY = targetByStepY - pixelsMovedY;

      if (moveX != 0 || moveY != 0) {
        MouseInput::moveRelative(moveX, moveY);
        pixelsMovedX += moveX;
        pixelsMovedY += moveY;
      }

      while (g_timer.getTime() < stepTargetTime) {
      }
    }

    // If firing/aiming was released mid-shot, do not send the unissued tail
    // of that shot. Otherwise the cursor could continue moving after release.
    if (shotInterrupted) {
      resetBurstState();
      semiWaitingForRelease = false;
      Sleep(1);
      continue;
    }

    const int remainingX = totalPixelsX - pixelsMovedX;
    const int remainingY = totalPixelsY - pixelsMovedY;
    if (remainingX != 0 || remainingY != 0)
      MouseInput::moveRelative(remainingX, remainingY);

    while (g_timer.getTime() < shotTargetTime) {
    }

    double actualShotTime = g_timer.getTime();
    double actualElapsed = actualShotTime - lastShotActualTime;
    double timingError = actualElapsed - targetDelaySeconds;

    timingErrorAccumulator =
        (timingErrorAccumulator * 0.8f) + ((float)timingError * 0.2f);
    if (timingErrorAccumulator < -0.003f)
      timingErrorAccumulator = -0.003f;
    if (timingErrorAccumulator > 0.003f)
      timingErrorAccumulator = 0.003f;

    lastShotActualTime = actualShotTime;
    shotCount++;
    shotStartTime = shotTargetTime;

    if (isAuto) {
      index = (index + 1) % (int)cachedPattern.size();
    } else {
      semiLastShotTime = g_timer.getTime();
      semiWaitingForRelease = true;
      resetBurstState();
    }
  }
}
