// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../common.h"
#include "../placer.h"

namespace dagdp
{

struct GrassTileCoord
{
  int32_t x;
  int32_t z;
};

static_assert(sizeof(GrassTileCoord) == 8);

static inline bool operator<(const GrassTileCoord &a, const GrassTileCoord &b)
{
  const auto az = abs(a.z), bz = abs(b.z);
  if (az != bz)
    return az < bz;
  const auto ax = abs(a.x), bx = abs(b.x);
  if (ax != bx)
    return ax < bx;
  return false;
}

static inline bool operator==(const GrassTileCoord &a, const GrassTileCoord &b) { return (a.x == b.x) && (a.z == b.z); }

struct GrassGridVariant
{
  dag::RelocatableFixedVector<PlacerObjectGroup, 4> objectGroups;
  int biome;
  float height, heightVariance;
};

struct GrassGrid
{
  dag::Vector<GrassGridVariant> variants;

  float density = 0.0f;
  float tileWorldSize = FLT_MAX;
  dag::Vector<GrassTileCoord> tiles;
};

struct GrassBuilder
{
  dag::Vector<GrassGrid> grids;
  float grassMaxRange = FLT_MAX;
};

#if DAGOR_DBGLEVEL != 0
struct GrassDebug
{
  dag::Vector<GrassBuilder> builders;
};
#endif

struct GrassManager
{
  GrassBuilder currentBuilder; // Only valid while building a view.

#if DAGOR_DBGLEVEL != 0
  GrassDebug debug;
#endif
};

void create_grass_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const GrassManager &grass_manager, NodeInserter node_inserter);

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::GrassManager);
