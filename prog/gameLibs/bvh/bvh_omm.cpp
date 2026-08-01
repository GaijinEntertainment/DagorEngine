// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_omm.h"

#include <bvh/bvh_processors.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>

namespace bvh
{

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

static bool bvh_enable_omm = false;
static bool bvh_retain_omm_bake_results = false;
static uint32_t bvh_omm_data_array_budget = 0xFFFFFFFFu;

void set_omm_settings(bool enable, int data_array_budget, bool retain_bake_results)
{
  bvh_enable_omm = enable;
  bvh_omm_data_array_budget = data_array_budget <= 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(data_array_budget);
  bvh_retain_omm_bake_results = retain_bake_results;
}

bool init_omm_context(ContextId context_id)
{
  const auto &caps = d3d::get_driver_desc().caps;
  return bvh_enable_omm && (caps.hasRayTraceOpacityMicroMapTriangleArrays || caps.hasNvidiaRayTraceOpacityMicroMapTriangleArrays) &&
         render::omm::init(context_id->ommContext);
}

static bool get_omm_texcoord_format(uint32_t bvh_format, render::omm::TexCoordFormat &omm_format)
{
  switch (bvh_format)
  {
    case VSDT_FLOAT2: omm_format = render::omm::TexCoordFormat::UV32_FLOAT; return true;
    case VSDT_HALF2: omm_format = render::omm::TexCoordFormat::UV16_FLOAT; return true;
    case VSDT_SHORT2:
    case BufferProcessor::bvhAttributeShort2TC: omm_format = render::omm::TexCoordFormat::UV16_UNORM; return true;
    default: return false;
  }
}

static TEXTUREID get_omm_texture_id(const Mesh &mesh)
{
  if (mesh.materialType & MeshMeta::bvhMaterialImpostor)
    return mesh.albedoTextureId;
  return mesh.alphaTextureId != BAD_TEXTUREID ? mesh.alphaTextureId : mesh.albedoTextureId;
}

static TEXTUREID get_omm_texture_id(const MeshInfo &mesh)
{
  if (mesh.isImpostor)
    return mesh.albedoTextureId;
  return mesh.alphaTextureId != BAD_TEXTUREID ? mesh.alphaTextureId : mesh.albedoTextureId;
}

bool mesh_wants_omm(ContextId context_id, const Mesh &mesh)
{
  return context_id->ommEnabled && (mesh.materialType & MeshMeta::bvhMaterialAlphaTest) && get_omm_texture_id(mesh) != BAD_TEXTUREID;
}

// Same predicate for the pre-build MeshInfo, before its geometry buffers (and OmmBakeSource) exist.
static bool mesh_wants_omm(ContextId context_id, const MeshInfo &mesh)
{
  return context_id->ommEnabled && mesh.alphaTest && get_omm_texture_id(mesh) != BAD_TEXTUREID;
}

static bool is_omm_candidate(ContextId context_id, const MeshInfo &mesh)
{
  if (!mesh_wants_omm(context_id, mesh))
    return false;
  if (mesh.vertexSize == 0 || mesh.indexCount == 0)
    return false;

  if (mesh.isImpostor)
    return mesh.vertexProcessor != nullptr;

  render::omm::TexCoordFormat ommFormat;
  return mesh.texcoordOffset != MeshInfo::invalidOffset && get_omm_texcoord_format(mesh.texcoordFormat, ommFormat);
}

static bool is_valid_omm_bake_source(const OmmBakeSource &source)
{
  render::omm::TexCoordFormat ommFormat;
  return source.texCoordBuffer && source.indexBuffer && source.texCoordStrideInBytes > 0 && source.indexCount > 0 &&
         get_omm_texcoord_format(source.texCoordFormat, ommFormat);
}

static bool is_omm_candidate(ContextId context_id, const Mesh &mesh, const OmmBakeSource &source)
{
  return mesh_wants_omm(context_id, mesh) && is_valid_omm_bake_source(source);
}

template <typename Container>
static bool contains_omm_texture(const Container &textures, TEXTUREID tex_id)
{
  for (TEXTUREID texture : textures)
    if (texture == tex_id)
      return true;
  return false;
}

static bool add_omm_texture_wait_ref(ContextId context_id, TEXTUREID tex_id)
{
  auto [iter, inserted] = context_id->ommTextureWaitRefs.insert({tex_id, 0});
  if (inserted)
  {
    mark_managed_tex_lfu(tex_id);
    if (!acquire_managed_tex(tex_id))
    {
      context_id->ommTextureWaitRefs.erase(iter);
      return false;
    }
  }
  ++iter->second;
  return true;
}

static void release_omm_texture_wait_ref(ContextId context_id, TEXTUREID tex_id)
{
  auto iter = context_id->ommTextureWaitRefs.find(tex_id);
  G_ASSERT_RETURN(iter != context_id->ommTextureWaitRefs.end(), );
  if (--iter->second == 0)
  {
    release_managed_tex(tex_id);
    context_id->ommTextureWaitRefs.erase(iter);
  }
}

static bool sync_omm_texture_waits(ContextId context_id, uint64_t object_id,
  const dag::Vector<TEXTUREID, framemem_allocator> &waiting_textures)
{
  auto objectWaitIter = context_id->ommTextureWaitsByObject.find(object_id);
  if (objectWaitIter == context_id->ommTextureWaitsByObject.end() && waiting_textures.empty())
    return false;

  auto &objectWaits = context_id->ommTextureWaitsByObject[object_id].textures;
  for (size_t i = 0; i < objectWaits.size();)
  {
    if (contains_omm_texture(waiting_textures, objectWaits[i]))
    {
      ++i;
      continue;
    }

    release_omm_texture_wait_ref(context_id, objectWaits[i]);
    objectWaits.erase(objectWaits.begin() + i);
  }

  for (TEXTUREID texId : waiting_textures)
    if (!contains_omm_texture(objectWaits, texId) && add_omm_texture_wait_ref(context_id, texId))
      objectWaits.push_back(texId);

  if (objectWaits.empty())
  {
    context_id->ommTextureWaitsByObject.erase(object_id);
    return false;
  }

  return true;
}

void release_omm_texture_waits_for_object(ContextId context_id, uint64_t object_id)
{
  auto iter = context_id->ommTextureWaitsByObject.find(object_id);
  if (iter == context_id->ommTextureWaitsByObject.end())
    return;

  for (TEXTUREID texId : iter->second.textures)
    release_omm_texture_wait_ref(context_id, texId);
  context_id->ommTextureWaitsByObject.erase(iter);
}

OmmBakeSource make_omm_bake_source(ContextId context_id, const Mesh &mesh)
{
  if (mesh.texcoordOffset == MeshInfo::invalidOffset)
    return {};

  return {
    .texCoordBuffer = mesh.geometry.getVertexBuffer(context_id),
    .indexBuffer = mesh.geometry.getIndexBuffer(context_id),
    .texCoordFormat = mesh.texcoordFormat,
    .texCoordOffsetInBytes = mesh.geometry.vbOffset + mesh.texcoordOffset,
    .texCoordStrideInBytes = mesh.vertexStride,
    .indexFormatBytes = mesh.indexFormat,
    .indexCount = mesh.indexCount,
    .indexStrideInBytes = mesh.indexFormat,
    .indexBufferOffsetInBytes = mesh.startIndex * mesh.indexFormat,
  };
}

OmmBakeSource make_omm_bake_source(ContextId context_id, const Mesh &mesh, const MeshMeta &meta)
{
  if (meta.texcoordOffset == 0xFFu)
    return {};

  return {
    .texCoordBuffer = mesh.geometry.getVertexBuffer(context_id),
    .indexBuffer = mesh.geometry.getIndexBuffer(context_id),
    .texCoordFormat = meta.texcoordFormat,
    .texCoordOffsetInBytes = mesh.geometry.vbOffset + meta.texcoordOffset,
    .texCoordStrideInBytes = meta.vertexStride,
    .indexFormatBytes = mesh.indexFormat,
    .indexCount = mesh.indexCount,
    .indexStrideInBytes = mesh.indexFormat,
    .indexBufferOffsetInBytes = meta.startIndex * mesh.indexFormat,
  };
}

// Objects whose alpha source never reaches full quality must not stall in the add queue forever,
// so after this many failed attempts the object is admitted without OMM.
static constexpr uint32_t MAX_OMM_TEXTURE_WAIT_ATTEMPTS = 1000;

OmmTextureWait should_wait_for_omm_texture(ContextId context_id, uint64_t object_id, const ObjectInfo &object_info)
{
  dag::Vector<TEXTUREID, framemem_allocator> waitingTextures;

  for (const MeshInfo &mesh : object_info.meshes)
  {
    if (!is_omm_candidate(context_id, mesh))
      continue;

    const TEXTUREID texId = get_omm_texture_id(mesh);
    if (get_managed_res_cur_tql(texId) != get_managed_res_max_tql(texId))
    {
      prefetch_and_check_managed_texture_loaded(texId, true);
      mark_managed_tex_lfu(texId);
      if (!contains_omm_texture(waitingTextures, texId))
        waitingTextures.push_back(texId);
    }
  }

  if (!sync_omm_texture_waits(context_id, object_id, waitingTextures))
    return OmmTextureWait::Ready;

  auto &waits = context_id->ommTextureWaitsByObject[object_id];
  if (++waits.attempts <= MAX_OMM_TEXTURE_WAIT_ATTEMPTS)
    return OmmTextureWait::Wait;

  String textureNames;
  for (TEXTUREID texId : waits.textures)
    textureNames.aprintf(64, "%s%s", textureNames.empty() ? "" : ", ", get_managed_texture_name(texId));
  logerr("BVH object %llu gave up waiting for OMM textures [%s], building its BLAS without OMM.", object_id, textureNames.c_str());

  release_omm_texture_waits_for_object(context_id, object_id);
  return OmmTextureWait::GaveUp;
}

static bool needs_secondary_omm(const Mesh &mesh) { return mesh.hasSecondaryGeometry; }

static uint32_t get_omm_texcoord_extra_offset(const Mesh &mesh, const OmmBakeSource &source, int slot_id)
{
  return slot_id == OMM_SECONDARY_SLOT ? mesh.vertexCount * source.texCoordStrideInBytes : 0;
}

static uint64_t get_omm_data_size_for_subdivision(uint8_t subdivision_level, render::omm::Format format)
{
  const uint64_t microTriangleCount = 1ull << (uint32_t(subdivision_level) * 2u);
  const uint64_t bitsPerState = format == render::omm::Format::OC1_4_State ? 2ull : 1ull;
  return max<uint64_t>((microTriangleCount * bitsPerState) >> 3u, 4ull);
}

static uint8_t get_budgeted_omm_max_subdivision_level(uint32_t triangle_count, uint8_t max_subdivision_level,
  render::omm::Format format, uint32_t data_array_budget)
{
  if (data_array_budget == 0xFFFFFFFFu || triangle_count == 0)
    return max_subdivision_level;

  for (int level = max_subdivision_level; level > 0; --level)
    if (get_omm_data_size_for_subdivision(level, format) * triangle_count <= data_array_budget)
      return uint8_t(level);

  return 0;
}

static bool start_omm_bake(ContextId context_id, Mesh &mesh, const OmmBakeSource &source, int slot_id, bool in_delayed_sync_window)
{
  const TEXTUREID texId = get_omm_texture_id(mesh);
  BaseTexture *texture = acquire_managed_tex(texId);
  if (!texture)
    return false;

  render::omm::TexCoordFormat texCoordFormat;
  if (!get_omm_texcoord_format(source.texCoordFormat, texCoordFormat))
  {
    release_managed_tex(texId);
    return false;
  }

  render::omm::BakeInput input;
  input.alphaTexture = texture;
  input.texCoordBuffer = source.texCoordBuffer;
  input.indexBuffer = source.indexBuffer;
  input.texCoordFormat = texCoordFormat;
  input.texCoordOffsetInBytes = source.texCoordOffsetInBytes + get_omm_texcoord_extra_offset(mesh, source, slot_id);
  input.texCoordStrideInBytes = source.texCoordStrideInBytes;
  input.indexFormat = source.indexFormatBytes == 2 ? render::omm::IndexFormat::UINT16 : render::omm::IndexFormat::UINT32;
  input.indexCount = source.indexCount;
  input.indexStrideInBytes = source.indexStrideInBytes;
  input.indexBufferOffsetInBytes = source.indexBufferOffsetInBytes;
  input.globalFormat = render::omm::Format::OC1_2_State,
  input.alphaTextureChannel = (mesh.materialType & MeshMeta::bvhMaterialImpostor) || mesh.alphaTextureId == BAD_TEXTUREID ? 3 : 0;
  input.maxOutOmmArraySize = bvh_omm_data_array_budget;
  input.maxSubdivisionLevel = get_budgeted_omm_max_subdivision_level(source.indexCount / 3, input.maxSubdivisionLevel,
    input.globalFormat, input.maxOutOmmArraySize);

  if (mesh.materialType & MeshMeta::bvhMaterialImpostor)
  {
    input.runtimeSamplerDesc.addressingMode = d3d::AddressMode::Border;
    input.runtimeSamplerDesc.borderAlpha = 0.f;
  }

  // The bake fires a chain of interdependent compute dispatches. Inside a delayed-sync window their
  // write/read hazards resolve as one batch and cannot be ordered (see bvh.cpp), so break out and
  // run the bake with immediate sync, matching the BLAS-build handling there.
  if (in_delayed_sync_window)
    d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
  const bool dispatched = render::omm::begin_bake(context_id->ommContext, input, mesh.ommSlots[slot_id].bakeHandle);
  if (in_delayed_sync_window)
    d3d::driver_command(Drv3dCommand::DELAY_SYNC);
  release_managed_tex(texId);
  return dispatched;
}

static void poll_baking_omm_slot(ContextId context_id, Mesh::OmmSlot &slot)
{
  if (slot.state != Mesh::OmmState::Baking)
    return;

  slot.lastPollFrame = dagor_frame_no();
  const render::omm::ConsumeBakeResult result = render::omm::consume_bake(context_id->ommContext, slot.bakeHandle, slot.bakeResult);
  if (result == render::omm::ConsumeBakeResult::NotReady)
    return;
  if (result == render::omm::ConsumeBakeResult::Failed)
  {
    slot.bakeHandle = {};
    slot.state = Mesh::OmmState::Failed;
    return;
  }
  slot.state = Mesh::OmmState::Ready;
}

static bool start_new_omm_bake(ContextId context_id, uint64_t object_id, Mesh &mesh, const OmmBakeSource &source,
  bool in_delayed_sync_window, int slot_id = OMM_PRIMARY_SLOT)
{
  if (slot_id == OMM_SECONDARY_SLOT && !needs_secondary_omm(mesh))
    return true;

  Mesh::OmmSlot &slot = mesh.ommSlots[slot_id];
  Mesh::OmmState &state = slot.state;
  if (!is_omm_candidate(context_id, mesh, source) || state == Mesh::OmmState::Built || state == Mesh::OmmState::Failed)
    return true;

  if (state == Mesh::OmmState::None)
  {
    if (!render::omm::has_free_bake_slot(context_id->ommContext))
      return false;

    if (!start_omm_bake(context_id, mesh, source, slot_id, in_delayed_sync_window))
    {
      state = Mesh::OmmState::Failed;
      return true;
    }
    state = Mesh::OmmState::Baking;
    slot.lastPollFrame = dagor_frame_no();
    context_id->objectsWithBakingOmm.insert(object_id);
  }

  return false;
}

static bool build_omm_if_ready(Mesh &mesh, OmmBuildInfos &omm_builds, OmmBuildResults &omm_build_results, int slot_id,
  uint64_t object_id, uint32_t geometry_index)
{
  Mesh::OmmSlot &slot = mesh.ommSlots[slot_id];
  Mesh::OmmState &state = slot.state;
  render::omm::BakeResult &result = slot.bakeResult;
  UniqueOMM &omm = slot.omm;

  if (state != Mesh::OmmState::Ready)
    return true;

  if (
    !result.arrayData || !result.descArray || !result.indexBuffer || result.arrayBuildDescs.empty() || result.blasLinkageDescs.empty())
  {
    state = Mesh::OmmState::Failed;
    render::omm::clear_result(result);
    return true;
  }

  auto sizeInfo = render::omm::make_array_build_info(result, nullptr, 0, 0, RaytraceBuildFlags::FAST_TRACE);
  const raytrace::AccelerationStructureSizes sizes = d3d::raytrace::calculate_acceleration_structure_sizes(sizeInfo);
  if (!sizes.structureSizeInBytes)
  {
    state = Mesh::OmmState::Failed;
    render::omm::clear_result(result);
    return true;
  }

  omm = UniqueOMM::create_omm(sizes.structureSizeInBytes);
  HANDLE_LOST_DEVICE_STATE(omm, false);
  if (!omm)
  {
    state = Mesh::OmmState::Failed;
    render::omm::clear_result(result);
    return true;
  }

  uint32_t scratchOffset = 0;
  Sbuffer *scratchBuffer = alloc_scratch_buffer(sizes.buildScratchBufferSizeInBytes, scratchOffset);
  if (sizes.buildScratchBufferSizeInBytes)
    HANDLE_LOST_DEVICE_STATE(scratchBuffer, false);

  raytrace::BatchedOpacityMicroMapTriangleArrayBuildInfo build;
  build.omm = omm.get();
  build.ommtabi = render::omm::make_array_build_info(result, scratchBuffer, scratchOffset, sizes.buildScratchBufferSizeInBytes,
    RaytraceBuildFlags::FAST_TRACE);
  omm_builds.push_back(build);
  omm_build_results.push_back(&result);
  state = Mesh::OmmState::Built;

  if (bvh_retain_omm_bake_results)
  {
    String debugLabel(0, "object=%llu geometry=%u slot=%u%s", static_cast<unsigned long long>(object_id), geometry_index,
      uint32_t(slot_id), slot_id == OMM_SECONDARY_SLOT ? " secondary" : "");
    render::omm::DebugBakeResultInfo debugInfo;
    debugInfo.label = debugLabel.c_str();
    debugInfo.objectId = object_id;
    debugInfo.geometryIndex = geometry_index;
    debugInfo.slotId = slot_id;
    debugInfo.materialType = mesh.materialType;
    debugInfo.impostor = mesh.materialType & MeshMeta::bvhMaterialImpostor;
    debugInfo.secondary = slot_id == OMM_SECONDARY_SLOT;
    render::omm::debug_register_bake_result(result, debugInfo);
  }

  return true;
}

bool start_new_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, const OmmBakeSource &source,
  bool in_delayed_sync_window)
{
  if (!start_new_omm_bake(context_id, object_id, mesh, source, in_delayed_sync_window, OMM_PRIMARY_SLOT))
    return false;
  if (
    needs_secondary_omm(mesh) && !start_new_omm_bake(context_id, object_id, mesh, source, in_delayed_sync_window, OMM_SECONDARY_SLOT))
    return false;

  return true;
}

static void consume_mesh_omm_slot(ContextId context_id, uint64_t object_id, Mesh &mesh, int slot_id, uint32_t geometry_index,
  OmmBuildInfos &build_infos, OmmBuildResults &build_results)
{
  poll_baking_omm_slot(context_id, mesh.ommSlots[slot_id]);
  build_omm_if_ready(mesh, build_infos, build_results, slot_id, object_id, geometry_index);
}

void consume_mesh_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, uint32_t geometry_index, OmmBuildInfos &build_infos,
  OmmBuildResults &build_results)
{
  consume_mesh_omm_slot(context_id, object_id, mesh, OMM_PRIMARY_SLOT, geometry_index, build_infos, build_results);
  if (needs_secondary_omm(mesh))
    consume_mesh_omm_slot(context_id, object_id, mesh, OMM_SECONDARY_SLOT, geometry_index, build_infos, build_results);
}

void consume_ready_omm_bakes(ContextId context_id, OmmBuildInfos &build_infos, OmmBuildResults &build_results)
{
  if (!context_id->ommEnabled)
    return;

  for (uint64_t objectId : context_id->objectsWithBakingOmm)
  {
    if (!context_id->halfBakedObjects.count(objectId))
      continue;

    Object *object = find_half_baked_object(context_id, objectId);
    if (!object)
      continue;

    for (uint32_t geometryIndex = 0; auto &mesh : object->meshes)
    {
      const uint32_t meshGeometryIndex = geometryIndex;
      geometryIndex += mesh.hasSecondaryGeometry ? 2 : 1;

      consume_mesh_omm_bakes(context_id, objectId, mesh, meshGeometryIndex, build_infos, build_results);
    }
  }
}

void set_omm_linkage(RaytraceGeometryDescription &desc, Mesh &mesh, int slot_id)
{
  Mesh::OmmSlot &slot = mesh.ommSlots[slot_id];
  Mesh::OmmState &state = slot.state;
  render::omm::BakeResult &result = slot.bakeResult;
  UniqueOMM &omm = slot.omm;

  if (state == Mesh::OmmState::Built && omm)
  {
    desc.ommLinkage = render::omm::make_geometry_linkage(result, omm.get());
    desc.extraDataAvailableMask.hasOpacityMicroMapLinkage = true;
  }
}

static void discard_baking_omm_slot(ContextId context_id, Mesh::OmmSlot &slot)
{
  if (slot.state != Mesh::OmmState::Baking)
    return;

  render::omm::discard_bake(context_id->ommContext, slot.bakeHandle);
  render::omm::clear_result(slot.bakeResult);
  slot.bakeHandle = {};
  slot.state = Mesh::OmmState::None;
}

void discard_inactive_omm_bakes(ContextId context_id)
{
  if (!context_id->ommEnabled)
    return;

  // A bake that no longer belongs to a live object would hold a pending bake slot forever, so it must
  // be discarded. Liveness is judged differently per source: half-baked objects are alive as long as
  // they are queued in halfBakedObjects (their resolve poll is budget-limited, so lastPollFrame can
  // lag without the bake being inactive); dynamic bakes have no such queue, but add_instances polls
  // them every frame -- with no budget gate -- while a live instance exists, so a stale poll stamp
  // there does mean the object went inactive. The tracked set is a superset of objects with baking
  // slots and is cleaned up here as bakes finish or objects vanish.
  constexpr uint32_t inactiveFrameThreshold = 2;
  for (auto iter = context_id->objectsWithBakingOmm.begin(); iter != context_id->objectsWithBakingOmm.end();)
  {
    const uint64_t objectId = *iter;
    Object *object = nullptr;
    if (auto objectIter = context_id->objects.find(objectId); objectIter != context_id->objects.end())
      object = &objectIter->second;
    else if (auto impostorIter = context_id->impostors.find(objectId); impostorIter != context_id->impostors.end())
      object = &impostorIter->second;
    if (!object)
    {
      iter = context_id->objectsWithBakingOmm.erase(iter);
      continue;
    }

    const bool stillQueuedForBuild = context_id->halfBakedObjects.count(objectId) != 0;
    bool hasBaking = false;
    bool hasStale = false;
    for (Mesh &mesh : object->meshes)
      for (Mesh::OmmSlot &slot : mesh.ommSlots)
        if (slot.state == Mesh::OmmState::Baking)
        {
          hasBaking = true;
          if (!stillQueuedForBuild && dagor_frame_no() - slot.lastPollFrame > inactiveFrameThreshold)
            hasStale = true;
        }

    if (hasStale)
      for (Mesh &mesh : object->meshes)
        for (Mesh::OmmSlot &slot : mesh.ommSlots)
          discard_baking_omm_slot(context_id, slot);

    if (!hasBaking || hasStale)
      iter = context_id->objectsWithBakingOmm.erase(iter);
    else
      ++iter;
  }
}

void release_omm_bake_build_inputs(OmmBuildResults &omm_build_results)
{
  if (bvh_retain_omm_bake_results)
  {
    omm_build_results.clear();
    return;
  }

  for (render::omm::BakeResult *result : omm_build_results)
  {
    if (!result)
      continue;

    result->arrayData.close();
    result->descArray.close();
    result->arrayDataSizeInBytes = 0;
    result->descArraySizeInBytes = 0;
  }
  omm_build_results.clear();
}

void build_pending_omm_arrays(OmmBuildInfos &omm_builds, OmmBuildResults &omm_build_results)
{
  if (omm_builds.empty())
    return;

  {
    TIME_D3D_PROFILE(bvh_build_omm_arrays);
    d3d::raytrace::build_acceleration_structure({
      .opacityMicroMapTriangleArrayBuilds = omm_builds,
      .flushAfterOpacityMicroMapTriangleArrayBuilds = true,
    });
  }
  omm_builds.clear();
  release_omm_bake_build_inputs(omm_build_results);
}

} // namespace bvh
