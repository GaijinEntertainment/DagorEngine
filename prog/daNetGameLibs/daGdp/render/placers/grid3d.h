// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../common.h"
#include "../placer.h"
#include "../../shaders/dagdp_volume.hlsli"
#include "../../shaders/dagdp_grid3d_placer.hlsli"

namespace dagdp
{

struct Grid3dPlacer
{
  dag::RelocatableFixedVector<PlacerObjectGroup, 4> objectGroups;

  float density = 0.0f;
  int prngSeed = 0;
  float gridJitter = 0.0f;
  float worldStep = FLT_MAX;
  float maxDrawDistance = 0;
  int csmCascadeCount = -1;
  Point2 distBasedScale = Point2(1, 1);
  Point3 distBasedCenter = Point3(0, 0, 0);
  float distBasedRange = 1;
  Point3 axis;
  bool axisLocal = false;
  bool axisAbs = false;
};

using Grid3dMapping = dag::VectorMap<ecs::EntityId, uint32_t>;

using Grid3dTiles = dag::RelocatableFixedVector<Grid3dTile, 256, true, framemem_allocator>;
using Grid3dVolumes = dag::RelocatableFixedVector<VolumeGpuData, 16, true, framemem_allocator>;
using Grid3dBoxes = dag::RelocatableFixedVector<bbox3f, 16, true, framemem_allocator>;

struct Grid3dBuilder
{
  dag::Vector<Grid3dPlacer> placers;
  Grid3dMapping mapping;
};

struct Grid3dManager
{
  Grid3dBuilder currentBuilder; // Only valid while building a view.
};

void gather(dag::ConstSpan<Grid3dPlacer> placers,
  const Grid3dMapping &placer_mapping,
  const ViewInfo &view_info,
  uint32_t viewport_index,
  const Viewport &viewport,
  float max_bounding_radius,
  Grid3dVolumes &out_volumes,
  Grid3dBoxes &out_volume_boxes);

void create_grid3d_nodes(const ViewInfo &view_info,
  const ViewBuilder &view_builder,
  const RulesBuilder &rules_builder,
  const Grid3dManager &grid3d_manager,
  NodeInserter node_inserter);

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::Grid3dManager);
