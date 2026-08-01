// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/dataComponent.h>
#include <daECS/core/internal/archetypes.h>

namespace ecs
{

// Fibonacci hash to distribute component_index_t across 64 bits for bloom filter.
// Sole definition of the bloom bit mapping: archetype masks (addArchetype) and query
// masks (computeQueryRequiredBitmask) must agree bit-for-bit for the prefilter to be sound.
static inline uint64_t componentBit(component_index_t comp) { return 1ULL << ((uint32_t(comp) * 0x9E3779B9u) >> 26); }

} // namespace ecs
