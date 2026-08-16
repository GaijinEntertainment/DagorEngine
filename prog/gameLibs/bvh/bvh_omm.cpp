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

#include <EASTL/utility.h>

namespace bvh
{

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

static bool bvh_enable_omm = false;
static bool bvh_retain_omm_bake_results = false;
static bool bvh_strict_asset_checks = false;
static uint32_t bvh_omm_data_array_budget = 0xFFFFFFFFu;

void set_omm_settings(const AdditionalSettings &settings)
{
  bvh_enable_omm = settings.enableOmm;
  bvh_omm_data_array_budget = settings.ommDataArrayBudget <= 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(settings.ommDataArrayBudget);
  bvh_retain_omm_bake_results = settings.retainOmmBakeResults;
  bvh_strict_asset_checks = settings.strictAssetChecks;
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
    case VSDT_FLOAT2: omm_format = render::omm::TexCoordFormat::Float2; return true;
    case VSDT_HALF2: omm_format = render::omm::TexCoordFormat::Half2; return true;
    case VSDT_USHORT2N: omm_format = render::omm::TexCoordFormat::UShort2Norm; return true;
    case BufferProcessor::bvhAttributeShort2TC: omm_format = render::omm::TexCoordFormat::Short2Fixed4096; return true;
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

static bool omm_texture_at_max_quality(TEXTUREID tex_id) { return get_managed_res_cur_tql(tex_id) == get_managed_res_max_tql(tex_id); }

OmmTextureWait should_wait_for_omm_texture(ContextId context_id, uint64_t object_id, const ObjectInfo &object_info)
{
  dag::Vector<TEXTUREID, framemem_allocator> waitingTextures;

  for (const MeshInfo &mesh : object_info.meshes)
  {
    if (!is_omm_candidate(context_id, mesh))
      continue;

    const TEXTUREID texId = get_omm_texture_id(mesh);
    if (!omm_texture_at_max_quality(texId))
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
  logerr("BVH object <%s> (id %llX) gave up waiting for OMM textures [%s]; it will be dropped from the BVH.",
    object_info.assetName.resolve().c_str(), object_id, textureNames.c_str());

  release_omm_texture_waits_for_object(context_id, object_id);
  return OmmTextureWait::GaveUp;
}

OmmTextureWait wait_for_grass_omm_texture(TEXTUREID alpha_tex_id, uint32_t &wait_attempts, String &give_up_reason)
{
  if (omm_texture_at_max_quality(alpha_tex_id))
    return OmmTextureWait::Ready;

  prefetch_and_check_managed_texture_loaded(alpha_tex_id, true);
  mark_managed_tex_lfu(alpha_tex_id);
  if (++wait_attempts <= MAX_OMM_TEXTURE_WAIT_ATTEMPTS)
    return OmmTextureWait::Wait;

  give_up_reason.printf(0, "its alpha texture '%s' never reached full quality", get_managed_texture_name(alpha_tex_id));
  return OmmTextureWait::GaveUp;
}

static bool needs_secondary_omm(const Mesh &mesh) { return mesh.hasSecondaryGeometry; }

static render::omm::UvCutout make_impostor_uv_cutout(const Mesh &mesh, int slot_id)
{
  const Point4 &lines = slot_id == OMM_SECONDARY_SLOT ? mesh.impostorSliceClippingLines1 : mesh.impostorSliceClippingLines2;

  const float leftAtV0 = lines.z;
  const float leftAtV1 = lines.x + lines.z;
  const float rightAtV0 = lines.w;
  const float rightAtV1 = lines.y + lines.w;

  if (leftAtV0 <= 0.f && leftAtV1 <= 0.f && rightAtV0 >= 1.f && rightAtV1 >= 1.f)
    return {};

  if (leftAtV0 >= rightAtV0 || leftAtV1 >= rightAtV1)
  {
    logerr("BVH impostor slot %d has an empty texcoord cutout band (%.3f..%.3f at v=0, %.3f..%.3f at v=1); "
           "baking its OMM without one, so the neighbouring atlas slice may leak into it.",
      slot_id, leftAtV0, rightAtV0, leftAtV1, rightAtV1);
    return {};
  }

  return {.lines = lines, .enabled = true};
}

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
  // Record before the early returns, thus the diagnostics can give the format even when it is the cause.
  mesh.ommSlots[slot_id].bakeTexcoordFormat = source.texCoordFormat;

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
  // Adaptive: dynamicSubdivisionScale picks a level for each triangle to get ~2x2 texel micro-triangles.
  // maxSubdivisionLevel is only the ceiling, which the budget can lower.
  input.maxSubdivisionLevel = get_budgeted_omm_max_subdivision_level(source.indexCount / 3, input.maxSubdivisionLevel,
    input.globalFormat, input.maxOutOmmArraySize);
  // In each build, and not only in a dev build: the counts are the only data that tells a fully opaque
  // bake from a fully transparent one, and the two are different asset problems.
  input.bakeFlags |= render::omm::ENABLE_POST_DISPATCH_INFO_STATS;

  if (mesh.materialType & MeshMeta::bvhMaterialImpostor)
  {
    input.runtimeSamplerDesc.addressingMode = d3d::AddressMode::Border;
    input.runtimeSamplerDesc.borderAlpha = 0.f;
    input.uvCutout = make_impostor_uv_cutout(mesh, slot_id);
    mesh.ommSlots[slot_id].bakeUvCutout = input.uvCutout.enabled;
  }

#if DAGOR_DBGLEVEL > 0
  mesh.ommSlots[slot_id].debugBakeSource = render::omm::make_debug_bake_source(input, texId);
#endif

  // The bake fires a chain of interdependent compute dispatches. Inside a delayed-sync window their
  // write/read hazards resolve as one batch and cannot be ordered (see bvh.cpp), so break out and
  // run the bake with immediate sync, matching the BLAS-build handling there.
  if (in_delayed_sync_window)
    d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
  const bool dispatched = render::omm::begin_bake(context_id->ommContext, input, mesh.ommSlots[slot_id].bakeHandle);
  // Only a dispatched bake gets a level, thus the diagnostics do not claim a bake that never ran.
  if (dispatched)
    mesh.ommSlots[slot_id].bakeSubdivisionLevel = input.maxSubdivisionLevel;
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
  const render::omm::ConsumeBakeResult result =
    render::omm::consume_bake(context_id->ommContext, slot.bakeHandle, slot.bakeResult, &slot.bakeStats);
  if (result == render::omm::ConsumeBakeResult::NotReady)
    return;
  if (result == render::omm::ConsumeBakeResult::Failed)
  {
    slot.bakeHandle = {};
    fail_omm_slot(slot, Mesh::OmmFailure::ReadbackInvalid);
    return;
  }
  slot.state = Mesh::OmmState::Ready;
}

static bool start_new_omm_bake(ContextId context_id, uint64_t object_id, Mesh &mesh, uint32_t geometry_index,
  const OmmBakeSource &source, bool in_delayed_sync_window, int slot_id = OMM_PRIMARY_SLOT)
{
  if (slot_id == OMM_SECONDARY_SLOT && !needs_secondary_omm(mesh))
    return true;

  Mesh::OmmSlot &slot = mesh.ommSlots[slot_id];
  Mesh::OmmState &state = slot.state;
  if (!is_omm_candidate(context_id, mesh, source) || state == Mesh::OmmState::Built || state == Mesh::OmmState::Failed)
    return true;

  if (state == Mesh::OmmState::None)
  {
    // The object wait covers only the add step: dynmodel instances reach here with no other gate.
    const TEXTUREID alphaTexId = get_omm_texture_id(mesh);
    if (!omm_texture_at_max_quality(alphaTexId))
    {
      // Every instance of the object polls this shared slot in each frame.
      if (slot.textureWaitFrame != dagor_frame_no())
      {
        slot.textureWaitFrame = dagor_frame_no();
        ++slot.textureWaitAttempts;
        prefetch_and_check_managed_texture_loaded(alphaTexId, true);
        mark_managed_tex_lfu(alphaTexId);
      }
      if (slot.textureWaitAttempts <= MAX_OMM_TEXTURE_WAIT_ATTEMPTS)
        return false;
      fail_omm_slot(slot, Mesh::OmmFailure::AlphaTextureNeverLoaded);
      publish_omm_debug_result(mesh, slot, object_id, geometry_index, slot_id, in_delayed_sync_window);
      return true;
    }

    if (!render::omm::has_free_bake_slot(context_id->ommContext))
      return false;

    if (!start_omm_bake(context_id, mesh, source, slot_id, in_delayed_sync_window))
    {
      fail_omm_slot(slot, Mesh::OmmFailure::BakeStartFailed);
      publish_omm_debug_result(mesh, slot, object_id, geometry_index, slot_id, in_delayed_sync_window);
      return true;
    }
    state = Mesh::OmmState::Baking;
    slot.lastPollFrame = dagor_frame_no();
    context_id->objectsWithBakingOmm.insert(object_id);
  }

  return false;
}

// Not "fullyOpaqueCount == triangleCount" on purpose: the SDK ignores a primitive whose micro-triangle
// counts are all zero, thus that equality gives a too small count for bad geometry.
bool bake_is_all_opaque(const render::omm::BakeStats &stats)
{
  return stats.totalFullyOpaqueCount > 0 && stats.totalFullyTransparentCount == 0 && stats.totalFullyUnknownCount == 0 &&
         stats.totalTransparentCount == 0 && stats.totalUnknownCount == 0;
}

bool bake_is_all_transparent(const render::omm::BakeStats &stats)
{
  return stats.totalFullyTransparentCount > 0 && stats.totalFullyOpaqueCount == 0 && stats.totalFullyUnknownCount == 0 &&
         stats.totalOpaqueCount == 0 && stats.totalUnknownCount == 0;
}

void publish_omm_debug_result(const Mesh &mesh, Mesh::OmmSlot &slot, uint64_t object_id, uint32_t geometry_index, int slot_id,
  bool in_delayed_sync_window)
{
  if (!bvh_retain_omm_bake_results)
    return;

  const bool failed = slot.state == Mesh::OmmState::Failed;

  // Mesh-level on purpose: a failed slot whose sibling disagrees still drops the object.
  if (failed && (mesh_should_be_opaque(mesh) || mesh_should_be_skipped(mesh)))
    return;

  const TEXTUREID texId = get_omm_texture_id(mesh);
  const char *texName = texId != BAD_TEXTUREID ? get_managed_texture_name(texId) : nullptr;
  const String label(0, "%s%s object=%llu geometry=%u slot=%u%s", failed ? "[FAILED] " : "", texName ? texName : "<no tex>",
    static_cast<unsigned long long>(object_id), geometry_index, uint32_t(slot_id), slot_id == OMM_SECONDARY_SLOT ? " secondary" : "");

  render::omm::DebugBakeResultInfo info;
#if DAGOR_DBGLEVEL > 0
  info.source = slot.debugBakeSource;
#endif
  info.label = label.c_str();
  info.objectId = object_id;
  info.geometryIndex = geometry_index;
  info.slotId = slot_id;
  info.materialType = mesh.materialType;
  info.impostor = (mesh.materialType & MeshMeta::bvhMaterialImpostor) != 0;
  info.secondary = slot_id == OMM_SECONDARY_SLOT;

  // The registration dispatches a compute copy of the texcoords, thus it must leave a delayed-sync
  // window as start_omm_bake does.
  if (in_delayed_sync_window)
    d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);

  if (!failed)
    render::omm::debug_register_bake_result(slot.bakeResult, info);
  else
  {
    info.failReason = omm_failure_text(slot.failure);
    render::omm::debug_adopt_bake_result(eastl::move(slot.bakeResult), info);
  }

  if (in_delayed_sync_window)
    d3d::driver_command(Drv3dCommand::DELAY_SYNC);
}

void publish_failed_grass_omm_debug_result(render::omm::BakeResult &result, const render::omm::DebugBakeSource &source,
  const char *label, const char *fail_reason)
{
  if (!bvh_retain_omm_bake_results)
    return;

  const String failLabel(0, "[FAILED] %s", label);
  render::omm::DebugBakeResultInfo info;
  info.source = source;
  info.label = failLabel.c_str();
  info.failReason = fail_reason;
  render::omm::debug_adopt_bake_result(eastl::move(result), info);
}

static bool build_omm_if_ready(Mesh &mesh, OmmBuildInfos &omm_builds, OmmBuildResults &omm_build_results, int slot_id,
  uint64_t object_id, uint32_t geometry_index, bool in_delayed_sync_window)
{
  Mesh::OmmSlot &slot = mesh.ommSlots[slot_id];
  Mesh::OmmState &state = slot.state;
  render::omm::BakeResult &result = slot.bakeResult;
  UniqueOMM &omm = slot.omm;

  if (state != Mesh::OmmState::Ready)
    return true;

  const auto fail = [&](Mesh::OmmFailure failure) {
    fail_omm_slot(slot, failure);
    // Publish before clear_result: the viewer can adopt the buffers.
    publish_omm_debug_result(mesh, slot, object_id, geometry_index, slot_id, in_delayed_sync_window);
    render::omm::clear_result(result);
    return true;
  };

  if (!result.arrayData || !result.descArray || !result.indexBuffer)
    return fail(Mesh::OmmFailure::NoOutputBuffers);

  if (result.arrayBuildDescs.empty() || result.blasLinkageDescs.empty())
    return fail(bake_is_all_opaque(slot.bakeStats)        ? Mesh::OmmFailure::AllTrianglesOpaque
                : bake_is_all_transparent(slot.bakeStats) ? Mesh::OmmFailure::AllTrianglesTransparent
                                                          : Mesh::OmmFailure::NoDescriptors);

  auto sizeInfo = render::omm::make_array_build_info(result, nullptr, 0, 0, RaytraceBuildFlags::FAST_TRACE);
  const raytrace::AccelerationStructureSizes sizes = d3d::raytrace::calculate_acceleration_structure_sizes(sizeInfo);
  if (!sizes.structureSizeInBytes)
    return fail(Mesh::OmmFailure::ZeroArraySize);

  omm = UniqueOMM::create_omm(sizes.structureSizeInBytes);
  HANDLE_LOST_DEVICE_STATE(omm, false);

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

  publish_omm_debug_result(mesh, slot, object_id, geometry_index, slot_id, in_delayed_sync_window);

  return true;
}

bool start_new_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, uint32_t geometry_index, const OmmBakeSource &source,
  bool in_delayed_sync_window)
{
  if (!start_new_omm_bake(context_id, object_id, mesh, geometry_index, source, in_delayed_sync_window, OMM_PRIMARY_SLOT))
    return false;
  if (needs_secondary_omm(mesh) &&
      !start_new_omm_bake(context_id, object_id, mesh, geometry_index, source, in_delayed_sync_window, OMM_SECONDARY_SLOT))
    return false;

  return true;
}

static void consume_mesh_omm_slot(ContextId context_id, uint64_t object_id, Mesh &mesh, int slot_id, uint32_t geometry_index,
  OmmBuildInfos &build_infos, OmmBuildResults &build_results, bool in_delayed_sync_window)
{
  Mesh::OmmSlot &slot = mesh.ommSlots[slot_id];
  const bool wasBaking = slot.state == Mesh::OmmState::Baking;
  poll_baking_omm_slot(context_id, slot);
  // A failed readback never reaches the publication in build_omm_if_ready: its state is not Ready.
  if (wasBaking && slot.state == Mesh::OmmState::Failed)
    publish_omm_debug_result(mesh, slot, object_id, geometry_index, slot_id, in_delayed_sync_window);
  build_omm_if_ready(mesh, build_infos, build_results, slot_id, object_id, geometry_index, in_delayed_sync_window);
}

void consume_mesh_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, uint32_t geometry_index, OmmBuildInfos &build_infos,
  OmmBuildResults &build_results, bool in_delayed_sync_window)
{
  consume_mesh_omm_slot(context_id, object_id, mesh, OMM_PRIMARY_SLOT, geometry_index, build_infos, build_results,
    in_delayed_sync_window);
  if (needs_secondary_omm(mesh))
    consume_mesh_omm_slot(context_id, object_id, mesh, OMM_SECONDARY_SLOT, geometry_index, build_infos, build_results,
      in_delayed_sync_window);
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

      // consume_ready_omm_bakes runs outside of a delayed-sync window
      consume_mesh_omm_bakes(context_id, objectId, mesh, meshGeometryIndex, build_infos, build_results, false);
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

bool mesh_omms_built(const Mesh &mesh)
{
  auto slotBuilt = [](const Mesh::OmmSlot &slot) { return slot.state == Mesh::OmmState::Built && slot.omm; };
  return slotBuilt(mesh.ommSlots[OMM_PRIMARY_SLOT]) && (!needs_secondary_omm(mesh) || slotBuilt(mesh.ommSlots[OMM_SECONDARY_SLOT]));
}

bool instance_can_use_mesh_omm(const Mesh &mesh, const MeshMeta &meta, const MeshMeta &base_meta)
{
  // process_meta initializes an uninitialized meta from base_meta, which has no override flag.
  if (!meta.isInitialized() || !(meta.materialType & MeshMeta::bvhMaterialUseInstanceTextures))
    return true;

  // An impostor bakes from its albedo texture, but its meta keeps the alpha one, thus the index
  // comparison below cannot tell whether the bake source is the same.
  if (mesh.materialType & MeshMeta::bvhMaterialImpostor)
    return false;

  // Context::holdTexture gives one bindless range to each TEXTUREID, thus two equal indices cannot be
  // two different textures.
  return meta.alphaTextureIndex != MeshMeta::INVALID_TEXTURE && meta.alphaTextureIndex == base_meta.alphaTextureIndex;
}

bool mesh_should_be_opaque(const Mesh &mesh)
{
  if (bvh_strict_asset_checks)
    return false;

  // All slots must agree: the flag applies to the full mesh, thus secondary geometry with a cutout blocks
  // this.
  auto slotOpaque = [](const Mesh::OmmSlot &slot) { return slot.failure == Mesh::OmmFailure::AllTrianglesOpaque; };
  return slotOpaque(mesh.ommSlots[OMM_PRIMARY_SLOT]) && (!needs_secondary_omm(mesh) || slotOpaque(mesh.ommSlots[OMM_SECONDARY_SLOT]));
}

bool mesh_should_be_skipped(const Mesh &mesh)
{
  if (bvh_strict_asset_checks)
    return false;

  auto slotTransparent = [](const Mesh::OmmSlot &slot) { return slot.failure == Mesh::OmmFailure::AllTrianglesTransparent; };
  return slotTransparent(mesh.ommSlots[OMM_PRIMARY_SLOT]) &&
         (!needs_secondary_omm(mesh) || slotTransparent(mesh.ommSlots[OMM_SECONDARY_SLOT]));
}

void make_mesh_opaque(Mesh &mesh, MeshMeta &base_meta)
{
  // mesh.materialType is what matters: the BLAS build reads it to mark the geometry IS_OPAQUE, and
  // mesh_wants_omm becomes false. The meta copy is cleared only to keep the two from disagreeing.
  mesh.materialType &= ~MeshMeta::bvhMaterialAlphaTest;
  base_meta.materialType &= ~MeshMeta::bvhMaterialAlphaTest;
}

void fail_omm_slot(Mesh::OmmSlot &slot, Mesh::OmmFailure failure)
{
  slot.state = Mesh::OmmState::Failed;
  slot.failure = failure;
}

const char *omm_failure_text(Mesh::OmmFailure failure)
{
  switch (failure)
  {
    case Mesh::OmmFailure::None: return "no reason was recorded";
    case Mesh::OmmFailure::UnsupportedTexcoordFormat: return "its texcoord packing is not one the OMM bake can decode";
    case Mesh::OmmFailure::NoAlphaSource: return "no alpha or albedo texture is available to bake an OMM from";
    case Mesh::OmmFailure::AlphaTextureNeverLoaded:
      return "its alpha source never reached full quality within the wait budget, so no OMM could be baked from it";
    case Mesh::OmmFailure::BakeStartFailed: return "the bake could not be started (alpha texture acquire or texcoord setup failed)";
    case Mesh::OmmFailure::ReadbackInvalid: return "the GPU bake readback returned no valid data";
    case Mesh::OmmFailure::NoOutputBuffers: return "the bake produced no output buffers";
    case Mesh::OmmFailure::NoDescriptors:
      return "the bake produced no OMM descriptors (every micro-triangle collapsed to a single state, and not all to opaque or all "
             "to transparent)";
    case Mesh::OmmFailure::AllTrianglesOpaque:
      return "its alpha source is fully opaque over the whole mesh, so the material should not be alpha-tested at all";
    case Mesh::OmmFailure::AllTrianglesTransparent:
      return "its alpha source is fully transparent over the whole mesh, so the raster draws none of it: delete this dead geometry";
    case Mesh::OmmFailure::ZeroArraySize: return "the OMM array reported a zero acceleration-structure size";
    case Mesh::OmmFailure::InstanceAlphaSourceOverride:
      return "this instance alpha-tests against a texture of its own, which no OMM is baked for yet: the mesh-shared "
             "OMM is baked from the mesh's own alpha source, and a correct one needs a per-override BLAS";
  }
  return "unrecognized failure";
}

// The shared texts tell the artist to remove the alpha test on a fully opaque bake, and to delete a
// fully transparent mesh. Grass is always a cutout and can do neither, thus both results mean that
// its alpha source is bad.
const char *grass_omm_failure_text(Mesh::OmmFailure failure)
{
  if (failure == Mesh::OmmFailure::AllTrianglesOpaque)
    return "its alpha source came back fully opaque, and grass is always a cutout -- the alpha texture is wrong";
  if (failure == Mesh::OmmFailure::AllTrianglesTransparent)
    return "its alpha source came back fully transparent, and grass is always a cutout -- the alpha texture is wrong";
  return omm_failure_text(failure);
}

static String bvh_texcoord_format_desc(uint32_t fmt)
{
  switch (fmt)
  {
    case VSDT_FLOAT2: return String("VSDT_FLOAT2");
    case VSDT_HALF2: return String("VSDT_HALF2");
    case VSDT_SHORT2: return String("VSDT_SHORT2");
    case VSDT_SHORT2N: return String("VSDT_SHORT2N");
    case VSDT_USHORT2N: return String("VSDT_USHORT2N");
    case BufferProcessor::bvhAttributeShort2TC: return String("bvhAttributeShort2TC");
    default: return String(0, "0x%X", fmt);
  }
}

static String describe_omm_bake_attempt(const Mesh::OmmSlot &slot)
{
  if (slot.bakeSubdivisionLevel == Mesh::OmmSlot::NO_BAKE_STARTED)
    return slot.bakeTexcoordFormat
             ? String(0, "; no bake was started, texcoords %s", bvh_texcoord_format_desc(slot.bakeTexcoordFormat).c_str())
             : String("; no bake was started");

  String desc(0, "; baked texcoords %s at a maximum subdivision level of %u%s",
    bvh_texcoord_format_desc(slot.bakeTexcoordFormat).c_str(), slot.bakeSubdivisionLevel,
    slot.bakeUvCutout ? " through a texcoord cutout" : "");
  if (slot.bakeSubdivisionLevel == 0)
    desc += " (the data array budget allowed no subdivision, so every triangle is a single micro-triangle "
            "and can only resolve to a uniform state)";

  // All counts zero means no bake completed, not that the bake found nothing.
  const render::omm::BakeStats &s = slot.bakeStats;
  if (s.totalOpaqueCount || s.totalTransparentCount || s.totalUnknownCount || s.totalFullyOpaqueCount ||
      s.totalFullyTransparentCount || s.totalFullyUnknownCount)
    desc.aprintf(0,
      " [triangles fully-opaque=%u fully-transparent=%u fully-unknown=%u; micro-triangles opaque=%u transparent=%u unknown=%u]",
      s.totalFullyOpaqueCount, s.totalFullyTransparentCount, s.totalFullyUnknownCount, s.totalOpaqueCount, s.totalTransparentCount,
      s.totalUnknownCount);
  return desc;
}

String describe_missing_omm(ContextId context_id, const Mesh &mesh)
{
  const TEXTUREID texId = get_omm_texture_id(mesh);
  const char *texName = texId != BAD_TEXTUREID ? get_managed_texture_name(texId) : nullptr;
  const String texDesc(0, "texture '%s'", texName ? texName : "<none>");

  if (!mesh_wants_omm(context_id, mesh))
    return String(0, "%s", omm_failure_text(Mesh::OmmFailure::NoAlphaSource));

  render::omm::TexCoordFormat unusedFormat;
  if (!get_omm_texcoord_format(mesh.texcoordFormat, unusedFormat))
    return String(0, "%s (%s); %s", omm_failure_text(Mesh::OmmFailure::UnsupportedTexcoordFormat),
      bvh_texcoord_format_desc(mesh.texcoordFormat).c_str(), texDesc.c_str());

  String failures;
  for (const Mesh::OmmSlot &slot : mesh.ommSlots)
    if (slot.state == Mesh::OmmState::Failed)
      failures.aprintf(0, "%s%s%s%s", failures.empty() ? "" : " / ",
        &slot == &mesh.ommSlots[OMM_SECONDARY_SLOT] ? "secondary geometry: " : "", omm_failure_text(slot.failure),
        describe_omm_bake_attempt(slot).c_str());
  if (!failures.empty())
    return String(0, "%s; %s", failures.c_str(), texDesc.c_str());

  return String(0, "the bake has not produced a usable result; %s%s", texDesc.c_str(),
    describe_omm_bake_attempt(mesh.ommSlots[OMM_PRIMARY_SLOT]).c_str());
}

static void discard_baking_omm_slot(ContextId context_id, Mesh::OmmSlot &slot)
{
  if (slot.state != Mesh::OmmState::Baking)
    return;

  render::omm::discard_bake(context_id->ommContext, slot.bakeHandle);
  render::omm::clear_result(slot.bakeResult);
  slot.bakeHandle = {};
  slot.state = Mesh::OmmState::None;
  slot.failureLogged = false;
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

Mesh::OmmFailure build_grass_omm_array(render::omm::BakeResult &result, const render::omm::BakeStats &stats, UniqueOMM &out_omm,
  OmmBuildInfos &build_infos, OmmBuildResults &build_results)
{
  if (!result.arrayData || !result.descArray || !result.indexBuffer)
    return Mesh::OmmFailure::NoOutputBuffers;
  if (result.arrayBuildDescs.empty() || result.blasLinkageDescs.empty())
    return bake_is_all_opaque(stats)        ? Mesh::OmmFailure::AllTrianglesOpaque
           : bake_is_all_transparent(stats) ? Mesh::OmmFailure::AllTrianglesTransparent
                                            : Mesh::OmmFailure::NoDescriptors;

  auto sizeInfo = render::omm::make_array_build_info(result, nullptr, 0, 0, RaytraceBuildFlags::FAST_TRACE);
  const raytrace::AccelerationStructureSizes sizes = d3d::raytrace::calculate_acceleration_structure_sizes(sizeInfo);
  if (!sizes.structureSizeInBytes)
    return Mesh::OmmFailure::ZeroArraySize;

  out_omm = UniqueOMM::create_omm(sizes.structureSizeInBytes);
  HANDLE_LOST_DEVICE_STATE(out_omm, Mesh::OmmFailure::None);

  uint32_t scratchOffset = 0;
  Sbuffer *scratchBuffer = alloc_scratch_buffer(sizes.buildScratchBufferSizeInBytes, scratchOffset);
  if (sizes.buildScratchBufferSizeInBytes)
    HANDLE_LOST_DEVICE_STATE(scratchBuffer, Mesh::OmmFailure::None);

  raytrace::BatchedOpacityMicroMapTriangleArrayBuildInfo build;
  build.omm = out_omm.get();
  build.ommtabi = render::omm::make_array_build_info(result, scratchBuffer, scratchOffset, sizes.buildScratchBufferSizeInBytes,
    RaytraceBuildFlags::FAST_TRACE);
  build_infos.push_back(build);
  build_results.push_back(&result);
  return Mesh::OmmFailure::None;
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
