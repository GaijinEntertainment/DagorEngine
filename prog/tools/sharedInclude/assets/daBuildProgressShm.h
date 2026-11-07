// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic.h>
#include <atomic>

struct DaBuildProgressShm
{
  volatile int gen;     // even = stable, odd = write in progress
  volatile int padding; // used to be phase, keeps field offsets
  volatile int packDone;
  volatile int packTotal;
  volatile int assetDone;
  volatile int assetTotal;

  void beginPackProgress()
  {
    interlocked_increment(gen);
    interlocked_relaxed_store(packDone, 0);
    interlocked_relaxed_store(packTotal, -1); // not started
    interlocked_relaxed_store(assetDone, 0);
    interlocked_relaxed_store(assetTotal, 0);
    interlocked_increment(gen);
  }

  void writePack(int done, int total)
  {
    interlocked_increment(gen);
    interlocked_relaxed_store(packDone, done);
    interlocked_relaxed_store(packTotal, total);
    interlocked_increment(gen);
  }

  void writeAsset(int done, int total)
  {
    interlocked_increment(gen);
    interlocked_relaxed_store(assetDone, done);
    interlocked_relaxed_store(assetTotal, total);
    interlocked_increment(gen);
  }

  void setAssetDone(int n) { interlocked_relaxed_store(assetDone, n); }

  void addAssetDone(int inc) { interlocked_add(assetDone, inc); }

  void readSnap(int &pd, int &pt, int &ad, int &at) const
  {
    int g1, g2;
    do
    {
      do
      {
        g1 = interlocked_acquire_load(gen);
      } while (g1 & 1);
      pd = interlocked_relaxed_load(packDone);
      pt = interlocked_relaxed_load(packTotal);
      ad = interlocked_relaxed_load(assetDone);
      at = interlocked_relaxed_load(assetTotal);
      std::atomic_thread_fence(std::memory_order_acq_rel);
      g2 = interlocked_relaxed_load(gen);
    } while (g1 != g2);
  }
};
