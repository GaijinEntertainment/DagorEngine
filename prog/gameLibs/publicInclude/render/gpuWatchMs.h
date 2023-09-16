//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>

class GPUWatchMs
{
  static uint64_t gpuFreq;

  void *startQuery = nullptr;
  void *endQuery = nullptr;

public:
  static void init_freq();
  static bool available();

  GPUWatchMs() = default;
  ~GPUWatchMs();

  void start();
  void stop();
  bool read(uint64_t &ov, uint64_t units = 1000);
};
