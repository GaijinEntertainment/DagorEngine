// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic.h>
#include <atomic>

struct DaBuildProgressShm
{
  volatile int gen;   // even = stable, odd = write in progress
  volatile int phase; // build phase 0 = none, 1 = tex packs, 2 = grp packs
  volatile int packDone;
  volatile int packTotal;
  volatile int assetDone;
  volatile int assetTotal;

  void writePack(int phase_, int done, int total, int a_done = 0, int a_total = 0)
  {
    interlocked_increment(gen);
    interlocked_relaxed_store(phase, phase_);
    interlocked_relaxed_store(packDone, done);
    interlocked_relaxed_store(packTotal, total);
    interlocked_relaxed_store(assetDone, a_done);
    interlocked_relaxed_store(assetTotal, a_total);
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

  void readSnap(int &ph, int &pd, int &pt, int &ad, int &at) const
  {
    int g1, g2;
    do
    {
      do
      {
        g1 = interlocked_acquire_load(gen);
      } while (g1 & 1);
      ph = interlocked_relaxed_load(phase);
      pd = interlocked_relaxed_load(packDone);
      pt = interlocked_relaxed_load(packTotal);
      ad = interlocked_relaxed_load(assetDone);
      at = interlocked_relaxed_load(assetTotal);
      std::atomic_thread_fence(std::memory_order_acq_rel);
      g2 = interlocked_relaxed_load(gen);
    } while (g1 != g2);
  }
};
