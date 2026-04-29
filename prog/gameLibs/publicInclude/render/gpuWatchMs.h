//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  GPUWatchMs(const GPUWatchMs &) = delete;
  GPUWatchMs &operator=(const GPUWatchMs &) = delete;
  GPUWatchMs(GPUWatchMs &&other) : startQuery(other.startQuery), endQuery(other.endQuery)
  {
    other.startQuery = nullptr;
    other.endQuery = nullptr;
  }
  GPUWatchMs &operator=(GPUWatchMs &&other)
  {
    if (this != &other)
    {
      startQuery = other.startQuery;
      endQuery = other.endQuery;
      other.startQuery = nullptr;
      other.endQuery = nullptr;
    }
    return *this;
  }
  ~GPUWatchMs();

  void start();
  void stop();
  bool read(uint64_t &ov, uint64_t units = 1000);
};
