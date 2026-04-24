// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/dataComponent.h>
#include <daECS/core/internal/archetypes.h>

namespace ecs
{

// Fibonacci hash to distribute component_index_t across 64 bits for bloom filter
static inline uint64_t componentBit(component_index_t comp) { return 1ULL << ((uint32_t(comp) * 0x9E3779B9u) >> 26); }

static inline uint64_t computeArchetypeBitmask(const Archetypes &archetypes, archetype_t arch)
{
  uint64_t bitmask = 0;
  const uint32_t compCount = archetypes.getArchetype(arch).getComponentsCount();
  for (uint32_t i = 0; i < compCount; ++i)
    bitmask |= componentBit(archetypes.getComponentUnsafe(arch, i));
  return bitmask;
}

} // namespace ecs
