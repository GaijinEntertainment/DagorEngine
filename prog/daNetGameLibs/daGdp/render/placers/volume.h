// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../common.h"
#include "../placer.h"
#include "../riexProcessor.h"
#include "../../shaders/dagdp_volume.hlsli"


namespace dagdp
{

static constexpr uint32_t ESTIMATED_RELEVANT_MESHES_PER_FRAME = 64;

struct VolumeVariant
{
  dag::RelocatableFixedVector<PlacerObjectGroup, 4> objectGroups;
  float density = 0.0;
};

struct VolumeMapping
{
  dag::VectorMap<ecs::EntityId, uint32_t> variantIds;
};

struct VolumeBuilder
{
  dag::Vector<VolumeVariant> variants;
  VolumeMapping mapping;
  uint32_t dynamicRegionIndex;
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

void gather_meshes(const VolumeMapping &volume_mapping,
  const ViewInfo &view_info,
  const Viewport &viewport,
  float max_bounding_radius,
  RiexProcessor &riex_processor,
  RelevantMeshes &out_result);

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::VolumeManager);
