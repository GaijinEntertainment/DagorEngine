// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>

#include <memory/dag_framemem.h>
#include <id/idIndexedFlags.h>
#include <id/idIndexedMapping.h>
#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>


namespace dafg
{

inline constexpr int SCHEDULE_FRAME_WINDOW = 2; // even and odd frames

struct LifetimePoint
{
  uint32_t frame;
  uint32_t timepoint;

  bool operator==(const LifetimePoint &other) const = default;
};

struct ResourceLifetime
{
  LifetimePoint firstUse;
  LifetimePoint release;

  bool operator==(const ResourceLifetime &other) const = default;
};

using ResourceLifetimes = eastl::array<IdIndexedMapping<intermediate::ResourceIndex, ResourceLifetime>, SCHEDULE_FRAME_WINDOW>;

class ResourceLifetimeCalculator
{
public:
  using LifetimesChanged = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

  LifetimesChanged recalculate(const intermediate::Graph &graph, const PassColoring &pass_coloring);

  const ResourceLifetimes &lifetimes() const { return resourceLifetimes; }

  void resetIncrementalState() { *this = ResourceLifetimeCalculator(); }

private:
  ResourceLifetimes resourceLifetimes;
};

} // namespace dafg
