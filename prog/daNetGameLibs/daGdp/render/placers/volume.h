// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include <math/dag_TMatrix.h>
#include <math/dag_frustum.h>
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
  AsyncNoWake, // queued only: a wake on the caller's path follows
  Synchronous
};

static constexpr float MIN_GEOMETRY_SIZE = 1.0f;

// (bmax+bmin) and (bmax-bmin) of a volume's culling box, the form Frustum::testBoxExtentB takes.
inline vec4f volume_cull_extent2(mat44f_cref tm44, float scale, float max_bounding_radius)
{
  return v_add(v_mul(v_add(v_abs(tm44.col0), v_add(v_abs(tm44.col1), v_abs(tm44.col2))), v_splats(2.0f * scale)),
    v_splats(2.0f * max_bounding_radius));
}

inline bool volume_in_draw_range(vec4f center, vec4f extent2, vec4f view_pos, float draw_radius)
{
  bbox3f volBox;
  v_bbox3_init_by_bsph(volBox, center, v_mul(extent2, V_C_HALF));
  bbox3f drawBox;
  v_bbox3_init_by_bsph(drawBox, view_pos, v_splats(draw_radius));
  return v_bbox3_test_box_intersect(drawBox, volBox);
}

// The volume itself: the unit cube scaled by +-scale. Doubles as the riex query box.
inline bbox3f volume_local_box(mat44f_cref tm44, float scale)
{
  bbox3f box;
  v_bbox3_init(box, tm44, {v_splats(-scale), v_splats(scale)});
  return box;
}

struct VolumeMappingItem;

struct DagdpRiexGatherJob : public cpujobs::IJob
{
  struct PlacerData
  {
    ecs::EntityId eid; // for validation
    size_t firstEntry = 0;
    size_t entryCount = 0;
    // The on_ri records only: the job filters the record's instances into meshPool.
    int meshLod = 0;
    int firstUnprocessedRes = -1; // riex resource of the first unprocessed instance, -1 if none
    size_t handleCount = 0;
    size_t firstMesh = 0; // into meshPool
    size_t meshCount = 0;
  };

  struct AroundRiPlacerData : PlacerData
  {
    const VolumeMappingItem *variant = nullptr;
    bbox3f fbox = {};
    float drawRadius = 0.0f;
    size_t firstLocalVolume = 0;
    size_t localVolumeCount = 0;
    size_t firstRiGenSource = 0; // into riGenSourcePool
    size_t riGenSourceCount = 0;
  };

  struct PerViewportData
  {
    Frustum frustum; // shrunk to the draw range, the one gather_process tests against
    bbox3f frustumBox = {};
    vec4f worldPos = {};
    bool valid = false;
    Tab<PlacerData> onRi;
    Tab<AroundRiPlacerData> aroundRi;
  };

  struct Entry
  {
    int resIdx;
    bbox3f box;
    Tab<rendinst::riex_handle_t> handles;
  };

  struct LocalVolumeSnapshot
  {
    TMatrix transform;
    float scale;
    int volumeType;
    // Filled by doJob: the entries of the (source, this volume) pairs that passed the draw range.
    size_t firstVolume = 0;
    size_t volumeCount = 0;
  };

  // The cull inputs come first so that rejecting a volume touches one cache line.
  struct VolumeEntry
  {
    vec4f center;
    vec4f extent2; // the full extent, as Frustum::testBoxExtentB takes it
    const VolumeMappingItem *variant;
    float scale;
    int volumeType;
    TMatrix tm;
    bbox3f box;    // in world space
    vec4f itmRow0; // the inverse divided by scale, the VolumeGpuData rows
    vec4f itmRow1;
    vec4f itmRow2;
    Point3 axis;
    int2 tileMin;
    int2 tileMax;
    float instancePosDelta;
    size_t firstMesh; // into meshPool
    size_t meshCount;
    int firstUnprocessedRes; // riex resource of the first unprocessed instance, -1 if none
  };

  // The job owns everything below between start() and waitDone(): read or reset it only after the drain.
  // Pools never shrink to preserve inner Tab allocations across frames
  Tab<Entry> entryPool;
  size_t entryCount = 0;
  Tab<PerViewportData> viewportDataPool;
  size_t viewportCount = 0;
  Tab<LocalVolumeSnapshot> localVolumePool;
  size_t localVolumeCount = 0;
  Tab<VolumeEntry> volumePool; // world volumes first, then the local volumes appended by doJob
  size_t volumeCount = 0;
  size_t worldVolumeCount = 0;
  // One arena for the filtered meshes of every volume and on_ri record: a Tab per volume would retain thousands of blocks.
  Tab<MeshIntersection> meshPool;
  Tab<TMatrix> riGenSourcePool; // RIGen sources have no riex handle, so gather_start snapshots their transforms
  Tab<TMatrix> sourceTmScratch;
  Tab<rendinst::riex_handle_t> handleScratch;
  Tab<MeshIntersection> meshScratch;

  // Inputs snapshotted at gather_start. gather_process culls with the same range and frustum, so every pre-gathered volume it
  // emits has pre-gathered meshes. The processor is only read here: its map changes in the process node, after the drain.
  const RiexProcessor *riexProcessor = nullptr;
  float rangeScale = 1.0f;
  float maxBoundingRadius = 0.0f;
  int targetMeshLod = 0;
  bool meshesEnabled = true;
  bool gridGathered = false;

  bool launched = false;
  bool scheduled = false;

  // Profiler names carry the view: every view runs the same code. The job's own name is preset in jobNameProfDesc.
  da_profiler::desc_id_t startDesc = 0;
  da_profiler::desc_id_t waitDesc = 0;

  ~DagdpRiexGatherJob() { waitDone(); } // views can die inside draw with the job in flight

  void initProfileMarkers(const char *view_name);
  void reset();
  PerViewportData &addViewport();
  void addEntries(PlacerData &rec, const ecs::IntList &resource_ids, bbox3f_cref box);
  LocalVolumeSnapshot &addLocalVolume();
  VolumeEntry &addVolumeEntry(const VolumeEntry &e);
  dag::ConstSpan<MeshIntersection> volumeMeshes(const VolumeEntry &e) const
  {
    return make_span_const(meshPool.data() + e.firstMesh, e.meshCount);
  }
  dag::ConstSpan<MeshIntersection> recordMeshes(const PlacerData &rec) const
  {
    return make_span_const(meshPool.data() + rec.firstMesh, rec.meshCount);
  }
  dag::ConstSpan<VolumeEntry> localVolumeSources(const LocalVolumeSnapshot &local) const
  {
    return make_span_const(volumePool.data() + local.firstVolume, local.volumeCount);
  }
  void start(GatherMode mode);
  void doJob() override;
  const char *getJobName(bool &) const override { return DAPROFILER_STRING("dagdp_volume_riex_gather"); }
  void waitDone();

private:
  size_t addEntry(int res_idx, bbox3f_cref box);
  void gatherVolume(VolumeEntry &e);
  void filterRecord(PlacerData &rec);
  void gatherLocalVolumes(AroundRiPlacerData &rec, size_t viewport_index);
};

void init_volume_entry(DagdpRiexGatherJob::VolumeEntry &e,
  const VolumeMappingItem *variant,
  const TMatrix &tm,
  mat44f_cref tm44,
  float scale,
  int volume_type,
  float max_bounding_radius);
void prepare_volume_entry(DagdpRiexGatherJob::VolumeEntry &e, mat44f_cref tm44);

// Appends the opaque elems of the handles that pass the placement filter, volumeIndex left to the caller. Skips the meshes
// of unprocessed resources and returns the riex resource index of the first of those, or -1.
int filter_volume_meshes(dag::ConstSpan<rendinst::riex_handle_t> handles,
  int mesh_lod,
  const bbox3f *volume_box,
  const RiexProcessor &riex_processor,
  Tab<MeshIntersection> &out_meshes);

static constexpr uint32_t ESTIMATED_LOCAL_VOLUMES_PER_PLACER = 4;
using LocalVolumeSnapshots =
  dag::RelocatableFixedVector<DagdpRiexGatherJob::LocalVolumeSnapshot, ESTIMATED_LOCAL_VOLUMES_PER_PLACER, true, framemem_allocator>;

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
  int csmCascadeCount;
};

using VolumeMapping = dag::VectorMap<ecs::EntityId, VolumeMappingItem>;

inline int resolve_mesh_lod(const VolumeMappingItem &variant, int view_target_lod)
{
  return variant.targetMeshLod >= 0 ? variant.targetMeshLod : view_target_lod;
}

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
  float max_bounding_radius,
  int target_mesh_lod,
  const RiexProcessor &riex_processor,
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

// dagdp_update_es may rebuild the views and free their jobs, so it must not start before that ES.
void start_gather_camera_volumes(const Point3 &cam_pos, const Frustum &cam_frustum);

// Reads the cascades ShadowsManager::prepareShadowsMatrices builds, so it must not start before that call.
void start_gather_csm_volumes(ecs::EntityManager &manager);

bool is_volume_early_riex_gather_enabled();

// Gathers the volume grid queries in the job as well. Has no effect while the riex gather above is off,
// which keeps the volume queries on the main thread as before.
bool is_volume_early_grid_gather_enabled();

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::VolumeManager);
