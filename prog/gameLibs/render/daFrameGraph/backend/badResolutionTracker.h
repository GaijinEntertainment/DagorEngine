// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/lruCache.h>
#include <common/dynamicResolution.h>
#include <math/integer/dag_IPoint3.h>

namespace dafg
{

class BadResolutionTracker
{
  static constexpr int SCHEDULE_FRAME_WINDOW = 2;

public:
  BadResolutionTracker(intermediate::Graph &graph) : graph(graph) {}

  void filterOutBadResolutions(DynamicResolutionUpdates &updates, int frame);

  bool pollRescheduling()
  {
    if (reschedulingCountdown == 0)
      return false;
    --reschedulingCountdown;
    return true;
  }

  using Corrections = eastl::array<IdIndexedMapping<intermediate::ResourceIndex, size_t, framemem_allocator>, SCHEDULE_FRAME_WINDOW>;
  Corrections getTexSizeCorrections() const;

private:
  ResourceDescription adjustDesc(const ResourceDescription &desc, const IPoint3 &resolution, float multiplier);
  size_t sizeFor(const ResourceDescription &desc);

private:
  intermediate::Graph &graph;

  struct Hasher
  {
    size_t operator()(const ResourceDescription &key) const { return key.hash(); }
  };
  LruCache<ResourceDescription, size_t, Hasher, 256> texSizeCache;
  struct Correction
  {
    eastl::array<size_t, SCHEDULE_FRAME_WINDOW> forFrame = {0};
  };
  ska::flat_hash_map<ResourceDescription, Correction, Hasher> correctedSizes;
  uint32_t reschedulingCountdown = 0;
};

} // namespace dafg
