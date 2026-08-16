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

// Frames to wait for an alpha source to reach full quality. Past it the wait fails the bake.
inline constexpr uint32_t MAX_OMM_TEXTURE_WAIT_ATTEMPTS = 1000;

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

OmmBakeSource make_omm_bake_source(ContextId context_id, const Mesh &mesh);
OmmBakeSource make_omm_bake_source(ContextId context_id, const Mesh &mesh, const MeshMeta &meta);

// True only for meshes that can actually carry an OMM (alpha-tested with an alpha texture). Lets
// process_meshes route opaque geometry straight to BLAS build instead of through the resolve stage.
bool mesh_wants_omm(ContextId context_id, const Mesh &mesh);

// Returns false while any slot still has pending bake work;
// once it returns true every slot is terminal (Built, Failed, or not a candidate) and stays so.
// in_delayed_sync_window must be true when called inside an open DELAY_SYNC window (see bvh.cpp): the
// bake fires interdependent compute dispatches that must break out of it to sync correctly.
bool start_new_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, uint32_t geometry_index, const OmmBakeSource &source,
  bool in_delayed_sync_window);

void consume_mesh_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, uint32_t geometry_index, OmmBuildInfos &build_infos,
  OmmBuildResults &build_results, bool in_delayed_sync_window);

void consume_ready_omm_bakes(ContextId context_id, OmmBuildInfos &build_infos, OmmBuildResults &build_results)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

void set_omm_linkage(RaytraceGeometryDescription &desc, Mesh &mesh, int slot_id = OMM_PRIMARY_SLOT);

// True if each OMM slot that the mesh links baked a usable OMM. In an OMM-capable context this must be
// true before alpha-tested geometry enters a BLAS, or the geometry is invisible.
bool mesh_omms_built(const Mesh &mesh);

// True if the mesh-shared OMM covers this instance. The code that withholds an instance and the code
// that links the OMM must both ask this and get the same answer, or an alpha-tested instance enters the
// BVH with an OMM baked from a texture it does not use.
bool instance_can_use_mesh_omm(const Mesh &mesh, const MeshMeta &meta, const MeshMeta &base_meta);

bool bake_is_all_opaque(const render::omm::BakeStats &stats);
bool bake_is_all_transparent(const render::omm::BakeStats &stats);

// True if the bake found that the mesh has no cutout, thus it can enter the BVH as opaque geometry.
// False when the strict asset checks are on: they report such an asset in place of a work-around.
bool mesh_should_be_opaque(const Mesh &mesh);

// True if the bake found the whole mesh transparent: the mesh keeps its geometry desc for the
// meta indexing but contributes no triangles. False when the strict asset checks are on.
bool mesh_should_be_skipped(const Mesh &mesh);

// Give it the meta of the object and not a copy of an instance: process_meta copies the flag from there
// to each instance.
void make_mesh_opaque(Mesh &mesh, MeshMeta &base_meta);

// The only function that sets an OMM slot to Failed, thus the cause is always recorded with the state.
void fail_omm_slot(Mesh::OmmSlot &slot, Mesh::OmmFailure failure);

// Shows the slot's bake in the viewer; call it after fail_omm_slot, because it reads slot.state and
// slot.failure. A failed slot's buffers move to the viewer, thus call it before clear_result. A bake
// that the mesh absorbs is not shown and keeps its buffers. No-op without retainOmmBakeResults.
void publish_omm_debug_result(const Mesh &mesh, Mesh::OmmSlot &slot, uint64_t object_id, uint32_t geometry_index, int slot_id,
  bool in_delayed_sync_window);

// The grass variant: grass owns no Mesh::OmmSlot, thus it hands over the result and the bake source
// directly. The same before-clear_result contract applies.
void publish_failed_grass_omm_debug_result(render::omm::BakeResult &result, const render::omm::DebugBakeSource &source,
  const char *label, const char *fail_reason);

const char *omm_failure_text(Mesh::OmmFailure failure);

// Shared by the two grass paths, thus they give the same name to the same cause.
const char *grass_omm_failure_text(Mesh::OmmFailure failure);

// Costly: call it only when the caller writes to the log.
String describe_missing_omm(ContextId context_id, const Mesh &mesh);

void discard_inactive_omm_bakes(ContextId context_id) DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

// Allocates and stages the OMM acceleration structure for a bake that no Mesh::OmmSlot owns: the two
// grass paths, whose geometry is outside the mesh pipeline.
Mesh::OmmFailure build_grass_omm_array(render::omm::BakeResult &result, const render::omm::BakeStats &stats, UniqueOMM &out_omm,
  OmmBuildInfos &build_infos, OmmBuildResults &build_results);

void build_pending_omm_arrays(OmmBuildInfos &build_infos, OmmBuildResults &build_results);
void release_omm_bake_build_inputs(OmmBuildResults &build_results);

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
