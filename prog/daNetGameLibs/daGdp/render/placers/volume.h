// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../common.h"
#include "../placer.h"
#include "../riexProcessor.h"
#include "../../shaders/dagdp_volume.hlsli"
#include "../../shaders/dagdp_volume_terrain.hlsli"


namespace dagdp
{

static constexpr uint32_t ESTIMATED_RELEVANT_MESHES_PER_FRAME = 256;
static constexpr uint32_t ESTIMATED_RELEVANT_TILES_PER_FRAME = 256;
static constexpr uint32_t ESTIMATED_RELEVANT_VOLUMES_PER_FRAME = 16;

struct VolumeVariant
{
  dag::RelocatableFixedVector<PlacerObjectGroup, 4> objectGroups;
  float density = 0.0f;
  float minTriangleArea = 0.0f;
};

struct VolumeMappingItem
{
  uint32_t variantIndex;
  float density;
};

using VolumeMapping = dag::VectorMap<ecs::EntityId, VolumeMappingItem>;

struct VolumeBuilder
{
  dag::Vector<VolumeVariant> variants;
  VolumeMapping mapping;
};

struct VolumeManager
{
  VolumeBuilder currentBuilder; // Only valid while building a view.
};

void create_volume_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const VolumeManager &volume_manager, NodeInserter node_inserter);

struct MeshToProcess
{
  int startIndex;
  int numFaces;
  int baseVertex;
  int stride;
  int vbIndex;
};

using RelevantMeshes = dag::RelocatableFixedVector<MeshIntersection, ESTIMATED_RELEVANT_MESHES_PER_FRAME>;
using RelevantTiles = dag::RelocatableFixedVector<VolumeTerrainTile, ESTIMATED_RELEVANT_TILES_PER_FRAME>;
using RelevantVolumes = dag::RelocatableFixedVector<VolumeGpuData, ESTIMATED_RELEVANT_VOLUMES_PER_FRAME>;

void gather(const VolumeMapping &volume_mapping,
  const ViewInfo &view_info,
  const Viewport &viewport,
  float max_bounding_radius,
  RiexProcessor &riex_processor,
  RelevantMeshes &out_meshes,
  RelevantTiles &out_tiles,
  RelevantVolumes &out_volumes);

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::VolumeManager);
