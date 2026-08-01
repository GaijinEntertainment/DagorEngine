// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_threadSafety.h>

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

void set_omm_settings(bool enable, int data_array_budget, bool retain_bake_results);
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
bool start_new_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, const OmmBakeSource &source,
  bool in_delayed_sync_window);

void consume_mesh_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, uint32_t geometry_index, OmmBuildInfos &build_infos,
  OmmBuildResults &build_results);

void consume_ready_omm_bakes(ContextId context_id, OmmBuildInfos &build_infos, OmmBuildResults &build_results)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

void set_omm_linkage(RaytraceGeometryDescription &desc, Mesh &mesh, int slot_id = OMM_PRIMARY_SLOT);

void discard_inactive_omm_bakes(ContextId context_id) DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

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

} // namespace bvh
