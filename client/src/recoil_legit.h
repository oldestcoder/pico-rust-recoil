#pragma once
#include "game_data.h"
#include <atomic>
#include <mutex>
#include <thread>

class LegitRecoil {
public:
  std::atomic<bool> active{false};

  void start();
  void stop();

private:
  std::mutex lock;
  std::thread recoilThread;
  void loop();
};

extern LegitRecoil g_legitEngine;
