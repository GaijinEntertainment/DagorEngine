//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <dag/dag_vector.h>
#include <stdint.h>

namespace da_profiler
{
struct UniqueEventData;
}

struct ProfileUniqueEventsGUI
{
  struct UniqueProfileDesc
  {
    const da_profiler::UniqueEventData *ued = nullptr;
    const char *name = nullptr;
    E3DCOLOR color = 0;
    uint32_t flushAtFrame = 0;
  };
  dag::Vector<UniqueProfileDesc> uniqueDescs;
  dag::Vector<uint16_t> profiledEvents;
  uint32_t uniqueFrames = 0, allFlushAt = 0;
  bool profileAll = false;
  void close()
  {
    uniqueDescs.clear();
    profiledEvents.clear();
  }
  void updateAll();
  void drawProfiled();
  void addProfile(const char *name, uint32_t flushAt = 1024);
  void flush(const char *name);
};
