// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_threadSafety.h>
#include <util/dag_string.h>

#include "bvh_context.h"

namespace bvh
{

inline constexpr int OMM_PRIMARY_SLOT = 0;
inline constexpr int OMM_SECONDARY_SLOT = 1;

struct OmmBakeSource
{
  Sbuffer *texCoordBuffer = nullptr;
  Sbuffer *indexBuffer = nullptr;
  uint32_t texCoordFormat = 0;
  uint32_t texCoordOffsetInBytes = 0;
  uint32_t texCoordStrideInBytes = 0;
  uint32_t indexFormatBytes = 0;
  uint32_t indexCount = 0;
  uint32_t indexStrideInBytes = 0;
  uint32_t indexBufferOffsetInBytes = 0;
};

using OmmBuildInfos = dag::Vector<raytrace::BatchedOpacityMicroMapTriangleArrayBuildInfo, framemem_allocator>;
using OmmBuildResults = dag::Vector<render::omm::BakeResult *, framemem_allocator>;

void set_omm_settings(const AdditionalSettings &settings);
bool init_omm_context(ContextId context_id);

// The bake path and the wait bypass must cover the same slots, thus both take the count from here.
bool has_secondary_geometry(const MeshInfo &mesh);

// mesh_index is the mesh ordinal, never the accumulated geometry index.
uint32_t omm_mesh_layout_hash(const MeshInfo &mesh, uint32_t mesh_index);

uint32_t omm_entry_bytes(const OmmCacheEntry &entry);

// Render thread only.
OmmEntryRef acquire_omm_entry(ContextId context_id, const OmmCacheKey &key);
OmmCacheEntry *find_omm_entry(ContextId context_id, const OmmCacheKey &key);

// The one place that decides whether a bake result is usable by a BLAS.
OmmCacheEntry *linkable_omm_entry(OmmCacheEntry *entry);
OmmCacheEntry *linkable_omm_entry(const Mesh &mesh, int slot_id = OMM_PRIMARY_SLOT);

// On the add that links the entry, restarts one that never started and retries a transient failure.
// The returned entry is borrowed from the mesh.
OmmCacheEntry *acquire_mesh_omm_entry(ContextId context_id, uint64_t object_id, Mesh &mesh, int slot_id);

// Render thread only, once per frame, after the frame's queued mesh actions: the refs a removal drops
// must be counted before the pass reads them.
void evict_idle_omm_entries(ContextId context_id);

OmmBakeSource make_omm_bake_source(ContextId context_id, const Mesh &mesh);
OmmBakeSource make_omm_bake_source(ContextId context_id, const Mesh &mesh, const MeshMeta &meta);

// True only for meshes that can actually carry an OMM (alpha-tested with an alpha texture). Lets
// process_meshes route opaque geometry straight to BLAS build instead of through the resolve stage.
bool mesh_wants_omm(ContextId context_id, const Mesh &mesh);

// Returns false while any slot still has pending bake work;
// once it returns true every slot is terminal (Built, Failed, or not a candidate) and stays so.
// in_delayed_sync_window must be true when called inside an open DELAY_SYNC window (see bvh.cpp): the
// bake fires interdependent compute dispatches that must break out of it to sync correctly.
bool start_new_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, const OmmBakeSource &source,
  bool in_delayed_sync_window) DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

void consume_mesh_omm_bakes(ContextId context_id, Mesh &mesh, OmmBuildInfos &build_infos, OmmBuildResults &build_results,
  bool in_delayed_sync_window);

enum class OverrideOmmResult
{
  Withhold,                    // the bake is on its way: report nothing
  WithholdAndReportOverride,   // no override bake is possible here: report InstanceAlphaSourceOverride
  WithholdAndReportBakeSource, // the mesh itself cannot be baked: report the mesh-level cause
  WithholdAndReportFailure,    // the override bake failed; out_entry carries the cause
  UseEntry                     // out_entry is terminal: built to link, or converted to per-instance geometry flags
};

// out_entry is borrowed and non-null only for UseEntry and WithholdAndReportFailure: the per-call cache
// reference is gone on return, thus it is valid only inside the current instance pass.
// out_override_tex_id is the resolved override texture, or BAD_TEXTUREID; the failure result always has one.
OverrideOmmResult resolve_override_omm_entry(ContextId context_id, uint64_t object_id, const Mesh &mesh, const MeshMeta &meta,
  const OmmBakeSource &source, bool in_delayed_sync_window, OmmCacheEntry *&out_entry, TEXTUREID &out_override_tex_id)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

void consume_ready_omm_bakes(ContextId context_id, OmmBuildInfos &build_infos, OmmBuildResults &build_results)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

// True while a bake that is not yet given up is in flight, thus a culled live object still counts.
bool has_active_omm_bakes(ContextId context_id) DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

// Links only a built entry with an OMM; any other leaves the desc without linkage.
void set_omm_linkage(RaytraceGeometryDescription &desc, Mesh &mesh, int slot_id = OMM_PRIMARY_SLOT);

void set_omm_linkage(RaytraceGeometryDescription &desc, OmmCacheEntry *entry);

// True if each OMM slot that the mesh links baked a usable OMM. In an OMM-capable context this must be
// true before alpha-tested geometry enters a BLAS, or the geometry is invisible.
bool mesh_omms_built(const Mesh &mesh);

// True if the mesh-shared OMM covers this instance. The code that withholds an instance and the code
// that links the OMM must both ask this and get the same answer, or an alpha-tested instance enters the
// BVH with an OMM baked from a texture it does not use.
bool instance_can_use_mesh_omm(const Mesh &mesh, const MeshMeta &meta, const MeshMeta &base_meta);

bool bake_is_all_opaque(const render::omm::BakeStats &stats);
bool bake_is_all_transparent(const render::omm::BakeStats &stats);

// A bake with no cutout (opaque) or no visible triangle (skipped) is a result, not a defect. False when
// the strict asset checks are on: they report in place of a work-around.
bool omm_entry_should_be_opaque(const OmmCacheEntry *entry);
bool omm_entry_should_be_skipped(const OmmCacheEntry *entry);

// True if the bake found that the mesh has no cutout, thus it can enter the BVH as opaque geometry.
bool mesh_should_be_opaque(const Mesh &mesh);

// True if the bake found the whole mesh transparent: the mesh keeps its geometry desc for the
// meta indexing but contributes no triangles. False when the strict asset checks are on.
bool mesh_should_be_skipped(const Mesh &mesh);

// Give it the meta of the object and not a copy of an instance: process_meta copies the flag from there
// to each instance.
void make_mesh_opaque(Mesh &mesh, MeshMeta &base_meta);

// The only function that sets an OMM entry to Failed, thus the cause is always recorded with the state.
void fail_omm_entry(ContextId context_id, OmmCacheEntry &entry, OmmFailure failure);

// Shows the entry's bake in the viewer. Call it after the entry is marked Failed (it reads the state and the
// failure) and before the result is cleared (a failed entry's buffers move to the viewer). A bake that the
// mesh absorbs is not shown and keeps its buffers. No-op without retainOmmBakeResults.
// label_tex_id names the bake texture in the label when it is not the mesh's own (the override bakes).
void publish_omm_debug_result(const Mesh &mesh, OmmCacheEntry &entry, uint64_t object_id, uint32_t geometry_index, int slot_id,
  bool in_delayed_sync_window, TEXTUREID label_tex_id = BAD_TEXTUREID);

// The grass variant: grass owns no OmmCacheEntry, thus it hands over the result and the bake source
// directly. The same before-clear contract applies.
void publish_failed_grass_omm_debug_result(render::omm::BakeResult &result, const render::omm::DebugBakeSource &source,
  const char *label, const char *fail_reason);

const char *omm_failure_text(OmmFailure failure);

// Shared by the two grass paths, thus they give the same name to the same cause.
const char *grass_omm_failure_text(OmmFailure failure);

// Costly: call it only when the caller writes to the log.
String describe_missing_omm(ContextId context_id, const Mesh &mesh, const OmmBakeSource &source);

void discard_inactive_omm_bakes(ContextId context_id) DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

// Allocates and stages the OMM acceleration structure for a bake that no OmmCacheEntry owns: the two
// grass paths, whose geometry is outside the mesh pipeline.
OmmFailure build_grass_omm_array(render::omm::BakeResult &result, const render::omm::BakeStats &stats, UniqueOMM &out_omm,
  OmmBuildInfos &build_infos, OmmBuildResults &build_results);

// A buffer the debug viewer may still show must never reach the next bake, thus nothing is recycled
// while the viewer is on.
void release_omm_result(render::omm::Context &omm_ctx, render::omm::BakeResult &result);

void build_pending_omm_arrays(render::omm::Context &omm_ctx, OmmBuildInfos &build_infos, OmmBuildResults &build_results);
// The array data and the desc array are read only by the OMM array build, thus they go back to the omm
// context store right after it; the index buffer stays with the result for the BLAS linkage. Releases
// nothing while retainOmmBakeResults keeps the buffers for the debug viewer.
void release_omm_bake_build_inputs(render::omm::Context &omm_ctx, OmmBuildResults &build_results);

enum class OmmTextureWait
{
  Ready,
  Wait,
  GaveUp
};

OmmTextureWait should_wait_for_omm_texture(ContextId context_id, uint64_t object_id, const ObjectInfo &object_info);
void release_omm_texture_waits_for_object(ContextId context_id, uint64_t object_id);

// The single-texture variant for the two grass paths, which key the wait on the geometry and not
// on an object id. give_up_reason is written only on GaveUp.
OmmTextureWait wait_for_grass_omm_texture(TEXTUREID alpha_tex_id, uint32_t &wait_attempts, String &give_up_reason);

} // namespace bvh
