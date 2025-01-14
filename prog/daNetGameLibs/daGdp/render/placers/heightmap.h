// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../common.h"
#include "../placer.h"

namespace dagdp
{

static constexpr size_t UINT_BITS = CHAR_BIT * sizeof(uint32_t);
static constexpr size_t UINT4_BITS = CHAR_BIT * sizeof(uint4);

struct HeightmapTileCoord
{
  int32_t x;
  int32_t z;
};

static_assert(sizeof(HeightmapTileCoord) == 8);

static inline bool operator<(const HeightmapTileCoord &a, const HeightmapTileCoord &b)
{
  if (a.z != b.z)
    return a.z < b.z;
  if (a.x != b.x)
    return a.x < b.x;
  return false;
}

static inline bool operator==(const HeightmapTileCoord &a, const HeightmapTileCoord &b) { return (a.x == b.x) && (a.z == b.z); }

struct HeightmapGridVariant
{
  dag::RelocatableFixedVector<int, 8> biomes;
  dag::RelocatableFixedVector<PlacerObjectGroup, 4> objectGroups;
  float effectiveDensity = 0.0f;
};

struct HeightmapGrid
{
  dag::Vector<HeightmapGridVariant> variants;

  float density = 0.0f;
  float tileWorldSize = FLT_MAX;
  int prngSeed = 0;
  float gridJitter = 0.0f;
  bool lowerLevel = false;
  bool useDynamicAllocation = false;
  dag::VectorSet<HeightmapTileCoord> tiles;
  dag::RelocatableFixedVector<uint32_t, 64> placeablesTileLimits;
};

struct HeightmapConfig
{
  float drawRangeLogBase = 2.0f;
};

struct HeightmapBuilder
{
  dag::Vector<HeightmapGrid> grids;
  SharedTex densityMask;
  Point4 maskScaleOffset = Point4(1.0f, 1.0f, 0.0f, 0.0f);
};

#if DAGOR_DBGLEVEL != 0
struct HeightmapDebug
{
  dag::Vector<HeightmapBuilder> builders;
};
#endif

struct HeightmapManager
{
  HeightmapConfig config;
  HeightmapBuilder currentBuilder; // Only valid while building a view.

#if DAGOR_DBGLEVEL != 0
  HeightmapDebug debug;
#endif
};

// Used to control memory strategy for heightmap placement.
// Dynamic allocation contains two phases: optimistic and pessimistic.
// If optimistic phase fails, the pessimistic phase is used after rearrangement.
enum class HeightmapPlacementType
{
  STATIC,
  DYNAMIC_OPTIMISTIC,
  DYNAMIC_PESSIMISTIC,
};

void create_heightmap_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const HeightmapManager &heightmap_manager, NodeInserter node_inserter);

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::HeightmapManager);
