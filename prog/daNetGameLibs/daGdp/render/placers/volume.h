// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <rendInst/riexHandle.h>
#include "../common.h"
#include "../placer.h"
#include "../riexProcessor.h"
#include "../../shaders/dagdp_volume.hlsli"
#include "../../shaders/dagdp_volume_terrain.hlsli"


namespace dagdp
{

enum class GatherMode
{
  Async,
  Synchronous
};

struct DagdpRiexGatherJob : public cpujobs::IJob
{
  struct PerViewportData
  {
    struct PlacerData
    {
      ecs::EntityId eid; // for validation
      size_t firstEntry;
      size_t entryCount;
    };
    bbox3f frustumBox;
    bool valid = false;
    Tab<PlacerData> onRi;
    Tab<PlacerData> aroundRi;
  };

  struct Entry
  {
    int resIdx;
    bbox3f box;
    Tab<rendinst::riex_handle_t> handles;
  };

  // Pools never shrink to preserve inner Tab allocations across frames
  Tab<Entry> entryPool;
  size_t entryCount = 0;
  Tab<PerViewportData> viewportDataPool;
  size_t viewportCount = 0;

  bool launched = false;
  bool scheduled = false;

  void reset();
  PerViewportData &addViewport();
  size_t addEntry(int res_idx, bbox3f_cref box);
  void start(GatherMode mode);
  void doJob() override;
  const char *getJobName(bool &) const override { return "dagdp_volume_riex_gather"; }
  void waitDone();
};

static constexpr uint32_t ESTIMATED_RELEVANT_MESHES_PER_FRAME = 256;
static constexpr uint32_t ESTIMATED_RELEVANT_TILES_PER_FRAME = 256;
static constexpr uint32_t ESTIMATED_RELEVANT_VOLUMES_PER_FRAME = 16;

struct VolumeVariant
{
  dag::RelocatableFixedVector<PlacerObjectGroup, 4> objectGroups;
  float density = 0.0f;
  float minTriangleArea = 0.0f;
  Point2 distBasedScale = Point2(1, 1);
  Point3 distBasedCenter = Point3(0, 0, 0);
  float distBasedRange = 1;
  float sampleRange = -1;
  bool axisAbs = false;
};

struct VolumeMappingItem
{
  uint32_t variantIndex;
  float density;
  float maxDrawDistance;
  int targetMeshLod;
  Point3 axis;
  bool axisLocal;
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

void create_volume_nodes(const ViewInfo &view_info,
  const ViewBuilder &view_builder,
  const VolumeManager &volume_manager,
  NodeInserter node_inserter,
  const RulesBuilder &rules_builder);

struct MeshToProcess
{
  int startIndex;
  int numFaces;
  int baseVertex;
  int stride;
  int vbIndex;
};

using RelevantMeshes = dag::RelocatableFixedVector<MeshIntersection, ESTIMATED_RELEVANT_MESHES_PER_FRAME, true, framemem_allocator>;
using RelevantTiles = dag::RelocatableFixedVector<VolumeTerrainTile, ESTIMATED_RELEVANT_TILES_PER_FRAME, true, framemem_allocator>;
using RelevantVolumes = dag::RelocatableFixedVector<VolumeGpuData, ESTIMATED_RELEVANT_VOLUMES_PER_FRAME, true, framemem_allocator>;

void gather_start(DagdpRiexGatherJob &job,
  const VolumeMapping &volume_mapping,
  const ViewInfo &view_info,
  const ViewPerFrameData &view_per_frame,
  GatherMode mode);

void gather_process(DagdpRiexGatherJob &job,
  const VolumeMapping &volume_mapping,
  const ViewInfo &view_info,
  uint32_t viewport_index,
  const Viewport &viewport,
  float max_bounding_radius,
  int target_mesh_lod,
  RiexProcessor &riex_processor,
  RelevantMeshes &out_meshes,
  RelevantTiles &out_tiles,
  RelevantVolumes &out_volumes);

void start_gather_before_draw_volumes(ecs::EntityManager &manager, const Point3 &cam_pos, const Frustum &cam_frustum);

bool is_volume_early_riex_gather_enabled();

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::VolumeManager);
