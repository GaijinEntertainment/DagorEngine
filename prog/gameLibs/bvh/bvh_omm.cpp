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

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>

namespace bvh
{

static bool bvh_enable_omm = false;
static bool bvh_retain_omm_bake_results = false;
static bool bvh_strict_asset_checks = false;
static uint32_t bvh_omm_data_array_budget = 0xFFFFFFFFu;
static uint32_t bvh_omm_cache_retention_frames = 900;
static uint64_t bvh_omm_cache_idle_budget = 64 << 20;
static uint32_t bvh_omm_texture_wait_attempts = 1000;

void set_omm_settings(const AdditionalSettings &settings)
{
  bvh_enable_omm = settings.enableOmm;
  bvh_omm_data_array_budget = settings.ommDataArrayBudget <= 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(settings.ommDataArrayBudget);
  bvh_retain_omm_bake_results = settings.retainOmmBakeResults;
  bvh_strict_asset_checks = settings.strictAssetChecks;
  bvh_omm_cache_retention_frames = max(settings.ommCacheRetentionFrames, 0);
  bvh_omm_cache_idle_budget = max(settings.ommCacheIdleBudget, 0);
  bvh_omm_texture_wait_attempts = max(settings.ommTextureWaitAttempts, 0);
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

bool has_secondary_geometry(const MeshInfo &mesh)
{
  return mesh.vertexProcessor && mesh.vertexProcessor->isGeneratingSecondaryVertices();
}

uint32_t omm_mesh_layout_hash(const MeshInfo &mesh, uint32_t mesh_index)
{
  uint32_t hash = 0x811C9DC5u;
  for (uint32_t field : {mesh_index, mesh.indexCount, mesh.vertexCount, mesh.texcoordFormat, mesh.texcoordOffset, mesh.vertexSize})
    hash = (hash ^ field) * 0x01000193u;
  return hash;
}

static OmmCacheKey make_omm_cache_key(const Mesh &mesh, uint64_t object_id, int slot_id, TEXTUREID bake_tex_id)
{
  return {object_id, uint32_t(slot_id), bake_tex_id, mesh.ommLayoutHash};
}

static OmmCacheKey make_omm_cache_key(const MeshInfo &mesh, uint64_t object_id, uint32_t mesh_index, int slot_id)
{
  return {object_id, uint32_t(slot_id), get_omm_texture_id(mesh), omm_mesh_layout_hash(mesh, mesh_index)};
}

// None of these is a property of the asset: the alpha source may load and a bake slot may free up.
static bool is_transient_omm_failure(OmmFailure failure)
{
  return failure == OmmFailure::AlphaTextureNeverLoaded || failure == OmmFailure::BakeStartFailed ||
         failure == OmmFailure::ReadbackInvalid;
}

// The resolve restarts such an entry and the wait bypass refuses to skip it, thus the two must agree.
static bool omm_entry_needs_bake(const OmmCacheEntry *entry)
{
  return !entry || entry->state == OmmState::None || (entry->state == OmmState::Failed && is_transient_omm_failure(entry->failure));
}

// Physical sizes, not the sizes the bake reported: the budget must count the VRAM the entry pins.
uint32_t omm_entry_bytes(const OmmCacheEntry &entry)
{
  auto bs = [](const UniqueBuf &buffer) { return buffer ? buffer->getSize() : 0u; };
  return entry.omm.getASSize() + bs(entry.bakeResult.indexBuffer) + bs(entry.bakeResult.arrayData) + bs(entry.bakeResult.descArray);
}

OmmEntryRef::OmmEntryRef(Context *context, OmmCacheEntry *entry) : context(context), entry(entry)
{
  if (entry)
    entry->refCount++;
}

void OmmEntryRef::reset()
{
  if (!entry)
    return;

  {
    OSSpinlockScopedLock lock(context->deferredOmmReleaseLock);
    context->deferredOmmReleases.push_back(entry);
  }
  context = nullptr;
  entry = nullptr;
}

// Only drops references, so the deferral can delay an eviction, never free a linked OMM too early.
static void drain_deferred_omm_releases(ContextId context_id)
{
  dag::Vector<OmmCacheEntry *> pending;
  {
    OSSpinlockScopedLock lock(context_id->deferredOmmReleaseLock);
    if (context_id->deferredOmmReleases.empty())
      return;
    pending.swap(context_id->deferredOmmReleases);
  }

  for (OmmCacheEntry *entry : pending)
    if (--entry->refCount == 0)
      entry->zeroRefFrame = dagor_frame_no();
}

OmmEntryRef acquire_omm_entry(ContextId context_id, const OmmCacheKey &key)
{
  return OmmEntryRef(context_id, &context_id->ommCache[key]);
}

OmmCacheEntry *find_omm_entry(ContextId context_id, const OmmCacheKey &key)
{
  auto iter = context_id->ommCache.find(key);
  return iter != context_id->ommCache.end() ? &iter->second : nullptr;
}

OmmCacheEntry *acquire_mesh_omm_entry(ContextId context_id, uint64_t object_id, Mesh &mesh, int slot_id)
{
  if (mesh.ommEntries[slot_id])
    return mesh.ommEntries[slot_id].get();

  OmmEntryRef ref = acquire_omm_entry(context_id, make_omm_cache_key(mesh, object_id, slot_id, get_omm_texture_id(mesh)));
  if (omm_entry_needs_bake(ref.get()))
    ref->resetBakeState();

  mesh.ommEntries[slot_id] = eastl::move(ref);
  return mesh.ommEntries[slot_id].get();
}

void release_omm_result(render::omm::Context &omm_ctx, render::omm::BakeResult &result)
{
  if (bvh_retain_omm_bake_results)
    render::omm::clear_result(result);
  else
    render::omm::recycle_result(omm_ctx, result, /*keep_index_buffer*/ false);
}

static void erase_baking_omm_entry(ContextId context_id, const OmmCacheEntry *entry)
{
  auto &entries = context_id->bakingOmmEntries;
  if (auto iter = eastl::find(entries.begin(), entries.end(), entry); iter != entries.end())
    entries.erase(iter);
}

// Gives the pending bake slot back. Leaves the bake result alone: a failed readback still publishes its
// buffers to the viewer.
static void abort_omm_entry_bake(ContextId context_id, OmmCacheEntry &entry)
{
  render::omm::discard_bake(context_id->ommContext, entry.bakeHandle);
  entry.bakeHandle = {};
  erase_baking_omm_entry(context_id, &entry);
}

// The entry starts over from None on its next resolve.
static void discard_omm_entry_bake(ContextId context_id, OmmCacheEntry &entry)
{
  abort_omm_entry_bake(context_id, entry);
  release_omm_result(context_id->ommContext, entry.bakeResult);
  entry.resetBakeState();
}

// Half-baked-object membership proves liveness where the resolve is budget-limited; elsewhere only the
// poll stamp does, and a live dynamic instance re-stamps it every frame.
static bool omm_bake_is_inactive(ContextId context_id, const OmmCacheEntry &entry, uint32_t inactive_frames)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  return context_id->halfBakedObjects.count(entry.bakeObjectId) == 0 && dagor_frame_no() - entry.lastPollFrame > inactive_frames;
}

// A live waiter polls its bake at least this often; a bigger lag means the object is gone.
static constexpr uint32_t OMM_BAKE_POLL_LAG_FRAMES = 2;
// Retry interval of a transient override bake failure; not the cache retention, which may be 0.
static constexpr uint32_t OMM_OVERRIDE_RETRY_FRAMES = 900;

// The poll stamp cannot tell a dead object from a live one whose instances are culled, thus a bake is
// given up only after this many unpolled frames.
static constexpr uint32_t OMM_BAKE_GIVE_UP_FRAMES = 60;

// A dead object's bake must not starve a live one: with the pool full, take one slot back at once in
// place of waiting out the periodic grace window.
static bool reclaim_inactive_omm_bake_slot(ContextId context_id) DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  for (OmmCacheEntry *entry : context_id->bakingOmmEntries)
    if (omm_bake_is_inactive(context_id, *entry, OMM_BAKE_POLL_LAG_FRAMES))
    {
      discard_omm_entry_bake(context_id, *entry);
      return true;
    }
  return false;
}

bool has_active_omm_bakes(ContextId context_id)
{
  for (const OmmCacheEntry *entry : context_id->bakingOmmEntries)
    if (!omm_bake_is_inactive(context_id, *entry, OMM_BAKE_GIVE_UP_FRAMES))
      return true;
  return false;
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

static void request_omm_texture(TEXTUREID tex_id)
{
  prefetch_and_check_managed_texture_loaded(tex_id, true);
  mark_managed_tex_lfu(tex_id);
  mark_managed_textures_important({&tex_id, 1});
}

// Wait bypass: an entry past state None has already passed this wait once. A transient failure does not
// count, so that its retry postpones the add in place of adding the object with the mesh withheld.
static bool omm_bakes_already_resolved(ContextId context_id, uint64_t object_id, uint32_t mesh_index, const MeshInfo &mesh)
{
  const int slotCount = has_secondary_geometry(mesh) ? 2 : 1;
  for (int slotId = 0; slotId < slotCount; ++slotId)
    if (omm_entry_needs_bake(find_omm_entry(context_id, make_omm_cache_key(mesh, object_id, mesh_index, slotId))))
      return false;
  return true;
}

OmmTextureWait should_wait_for_omm_texture(ContextId context_id, uint64_t object_id, const ObjectInfo &object_info)
{
  dag::Vector<TEXTUREID, framemem_allocator> waitingTextures;

  for (uint32_t meshIndex = 0; meshIndex < object_info.meshes.size(); ++meshIndex)
  {
    const MeshInfo &mesh = object_info.meshes[meshIndex];
    if (!is_omm_candidate(context_id, mesh) || omm_bakes_already_resolved(context_id, object_id, meshIndex, mesh))
      continue;

    const TEXTUREID texId = get_omm_texture_id(mesh);
    if (!omm_texture_at_max_quality(texId))
    {
      request_omm_texture(texId);
      if (!contains_omm_texture(waitingTextures, texId))
        waitingTextures.push_back(texId);
    }
  }

  if (!sync_omm_texture_waits(context_id, object_id, waitingTextures))
    return OmmTextureWait::Ready;

  auto &waits = context_id->ommTextureWaitsByObject[object_id];
  if (++waits.attempts <= bvh_omm_texture_wait_attempts)
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

  request_omm_texture(alpha_tex_id);
  if (++wait_attempts <= bvh_omm_texture_wait_attempts)
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

// The OMM must agree with the any-hit shader, which takes the channel from this flag: 0 is the red of a
// dedicated alpha texture, 3 the alpha of the albedo the alpha slot falls back to.
static uint32_t omm_alpha_channel(uint32_t material_type)
{
  // An impostor bakes from its albedo whatever its alpha slot holds, thus the flag cannot speak for it.
  if (material_type & MeshMeta::bvhMaterialImpostor)
    return 3;
  return (material_type & MeshMeta::bvhMaterialAlphaInRed) ? 0 : 3;
}

static bool start_omm_bake(ContextId context_id, const Mesh &mesh, OmmCacheEntry &entry, const OmmBakeSource &source, int slot_id,
  TEXTUREID bake_tex_id, uint32_t alpha_channel, bool in_delayed_sync_window)
{
  // Record before the early returns, thus the diagnostics can give the format even when it is the cause.
  entry.bakeTexcoordFormat = source.texCoordFormat;

  const TEXTUREID texId = bake_tex_id;
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
  input.globalFormat = render::omm::Format::OC1_2_State;
  input.alphaTextureChannel = alpha_channel;
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
    entry.bakeUvCutout = input.uvCutout.enabled;
  }

#if DAGOR_DBGLEVEL > 0
  entry.debugBakeSource = render::omm::make_debug_bake_source(input, texId);
#endif

  // The bake fires a chain of interdependent compute dispatches. Inside a delayed-sync window their
  // write/read hazards resolve as one batch and cannot be ordered (see bvh.cpp), so break out and
  // run the bake with immediate sync, matching the BLAS-build handling there.
  if (in_delayed_sync_window)
    d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
  const bool dispatched = render::omm::begin_bake(context_id->ommContext, input, entry.bakeHandle);
  // Only a dispatched bake gets a level, thus the diagnostics do not claim a bake that never ran.
  if (dispatched)
    entry.bakeSubdivisionLevel = input.maxSubdivisionLevel;
  if (in_delayed_sync_window)
    d3d::driver_command(Drv3dCommand::DELAY_SYNC);
  release_managed_tex(texId);
  return dispatched;
}

static void poll_baking_omm_entry(ContextId context_id, OmmCacheEntry &entry)
{
  if (entry.state != OmmState::Baking)
    return;

  const render::omm::ConsumeBakeResult result =
    render::omm::consume_bake(context_id->ommContext, entry.bakeHandle, entry.bakeResult, &entry.bakeStats);
  if (result == render::omm::ConsumeBakeResult::NotReady)
    return;
  if (result == render::omm::ConsumeBakeResult::Failed)
  {
    fail_omm_entry(context_id, entry, OmmFailure::ReadbackInvalid);
    return;
  }
  entry.state = OmmState::Ready;
  entry.bakeHandle = {};
  erase_baking_omm_entry(context_id, &entry);
}

enum class OmmBakeAdvance
{
  Waiting, // the texture is loading or no bake slot is free
  Failed,  // the failure is recorded on the entry and published
  Started  // the entry moved to Baking
};

// The None-to-Baking transition, shared by the mesh-shared and the override bake starts.
static OmmBakeAdvance advance_unbaked_omm_entry(ContextId context_id, uint64_t object_id, const Mesh &mesh, OmmCacheEntry &entry,
  const OmmBakeSource &source, int slot_id, TEXTUREID bake_tex_id, uint32_t alpha_channel, bool in_delayed_sync_window)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  if (!omm_texture_at_max_quality(bake_tex_id))
  {
    // Several resolves can poll the entry in one frame; count one wait attempt per frame.
    if (entry.textureWaitFrame != dagor_frame_no())
    {
      entry.textureWaitFrame = dagor_frame_no();
      ++entry.textureWaitAttempts;
      request_omm_texture(bake_tex_id);
    }
    if (entry.textureWaitAttempts <= bvh_omm_texture_wait_attempts)
      return OmmBakeAdvance::Waiting;
    fail_omm_entry(context_id, entry, OmmFailure::AlphaTextureNeverLoaded);
    entry.debugPublished = true;
    publish_omm_debug_result(mesh, entry, object_id, mesh.firstGeometryIndex, slot_id, in_delayed_sync_window, bake_tex_id);
    return OmmBakeAdvance::Failed;
  }

  if (!render::omm::has_free_bake_slot(context_id->ommContext) && !reclaim_inactive_omm_bake_slot(context_id))
    return OmmBakeAdvance::Waiting;

  if (!start_omm_bake(context_id, mesh, entry, source, slot_id, bake_tex_id, alpha_channel, in_delayed_sync_window))
  {
    fail_omm_entry(context_id, entry, OmmFailure::BakeStartFailed);
    entry.debugPublished = true;
    publish_omm_debug_result(mesh, entry, object_id, mesh.firstGeometryIndex, slot_id, in_delayed_sync_window, bake_tex_id);
    return OmmBakeAdvance::Failed;
  }

  entry.state = OmmState::Baking;
  entry.bakeObjectId = object_id;
  context_id->bakingOmmEntries.push_back(&entry);
  return OmmBakeAdvance::Started;
}

static bool start_new_omm_bake(ContextId context_id, uint64_t object_id, Mesh &mesh, const OmmBakeSource &source,
  bool in_delayed_sync_window, int slot_id = OMM_PRIMARY_SLOT) DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  if (slot_id == OMM_SECONDARY_SLOT && !needs_secondary_omm(mesh))
    return true;
  if (!is_omm_candidate(context_id, mesh, source))
    return true;

  OmmCacheEntry &entry = *acquire_mesh_omm_entry(context_id, object_id, mesh, slot_id);
  OmmState &state = entry.state;
  if (state == OmmState::Built || state == OmmState::Failed)
    return true;

  entry.lastPollFrame = dagor_frame_no();

  if (state == OmmState::None)
  {
    // The object wait covers only the add step: dynmodel instances reach here with no other gate.
    if (advance_unbaked_omm_entry(context_id, object_id, mesh, entry, source, slot_id, get_omm_texture_id(mesh),
          omm_alpha_channel(mesh.materialType), in_delayed_sync_window) == OmmBakeAdvance::Failed)
      return true;
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

void publish_omm_debug_result(const Mesh &mesh, OmmCacheEntry &entry, uint64_t object_id, uint32_t geometry_index, int slot_id,
  bool in_delayed_sync_window, TEXTUREID label_tex_id)
{
  if (!bvh_retain_omm_bake_results)
    return;

  const bool failed = entry.state == OmmState::Failed;

  // Mesh-level on purpose: a failed slot whose sibling disagrees still drops the object. Applies to the
  // mesh's own slots only; an override entry's absorb is decided at its resolve.
  if (failed && mesh.ommEntries[slot_id].get() == &entry && (mesh_should_be_opaque(mesh) || mesh_should_be_skipped(mesh)))
    return;

  const TEXTUREID texId = label_tex_id != BAD_TEXTUREID ? label_tex_id : get_omm_texture_id(mesh);
  const char *texName = texId != BAD_TEXTUREID ? get_managed_texture_name(texId) : nullptr;
  const String label(0, "%s%s object=%llu geometry=%u slot=%u%s", failed ? "[FAILED] " : "", texName ? texName : "<no tex>",
    static_cast<unsigned long long>(object_id), geometry_index, uint32_t(slot_id), slot_id == OMM_SECONDARY_SLOT ? " secondary" : "");

  render::omm::DebugBakeResultInfo info;
#if DAGOR_DBGLEVEL > 0
  info.source = entry.debugBakeSource;
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
    render::omm::debug_register_bake_result(entry.bakeResult, info);
  else
  {
    info.failReason = omm_failure_text(entry.failure);
    render::omm::debug_adopt_bake_result(eastl::move(entry.bakeResult), info);
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

// mesh is null when the object that started the bake is gone; the bake still finishes into the cache.
static bool build_omm_if_ready(ContextId context_id, OmmCacheEntry &entry, const Mesh *mesh, int slot_id, OmmBuildInfos &omm_builds,
  OmmBuildResults &omm_build_results, bool in_delayed_sync_window)
{
  OmmState &state = entry.state;
  render::omm::BakeResult &result = entry.bakeResult;

  if (state != OmmState::Ready)
    return true;

  const auto publish = [&] {
    if (mesh)
      publish_omm_debug_result(*mesh, entry, entry.bakeObjectId, mesh->firstGeometryIndex, slot_id, in_delayed_sync_window);
  };

  const auto fail = [&](OmmFailure failure) {
    fail_omm_entry(context_id, entry, failure);
    // Publish before the release: the viewer can adopt the buffers.
    publish();
    // No owner mesh: the override resolve publishes later, so the buffers must stay for it.
    if (mesh || !bvh_retain_omm_bake_results)
      release_omm_result(context_id->ommContext, result);
    return true;
  };

  if (!result.arrayData || !result.descArray || !result.indexBuffer)
    return fail(OmmFailure::NoOutputBuffers);

  if (result.arrayBuildDescs.empty() || result.blasLinkageDescs.empty())
    return fail(bake_is_all_opaque(entry.bakeStats)        ? OmmFailure::AllTrianglesOpaque
                : bake_is_all_transparent(entry.bakeStats) ? OmmFailure::AllTrianglesTransparent
                                                           : OmmFailure::NoDescriptors);

  auto sizeInfo = render::omm::make_array_build_info(result, nullptr, 0, 0, RaytraceBuildFlags::FAST_TRACE);
  const raytrace::AccelerationStructureSizes sizes = d3d::raytrace::calculate_acceleration_structure_sizes(sizeInfo);
  if (!sizes.structureSizeInBytes)
    return fail(OmmFailure::ZeroArraySize);

  entry.omm = UniqueOMM::create_omm(sizes.structureSizeInBytes);
  HANDLE_LOST_DEVICE_STATE(entry.omm, false);

  uint32_t scratchOffset = 0;
  Sbuffer *scratchBuffer = alloc_scratch_buffer(sizes.buildScratchBufferSizeInBytes, scratchOffset);
  if (sizes.buildScratchBufferSizeInBytes)
    HANDLE_LOST_DEVICE_STATE(scratchBuffer, false);

  raytrace::BatchedOpacityMicroMapTriangleArrayBuildInfo build;
  build.omm = entry.omm.get();
  build.ommtabi = render::omm::make_array_build_info(result, scratchBuffer, scratchOffset, sizes.buildScratchBufferSizeInBytes,
    RaytraceBuildFlags::FAST_TRACE);
  omm_builds.push_back(build);
  omm_build_results.push_back(&result);
  state = OmmState::Built;

  publish();

  return true;
}

bool start_new_omm_bakes(ContextId context_id, uint64_t object_id, Mesh &mesh, const OmmBakeSource &source,
  bool in_delayed_sync_window) DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  if (!start_new_omm_bake(context_id, object_id, mesh, source, in_delayed_sync_window, OMM_PRIMARY_SLOT))
    return false;
  if (
    needs_secondary_omm(mesh) && !start_new_omm_bake(context_id, object_id, mesh, source, in_delayed_sync_window, OMM_SECONDARY_SLOT))
    return false;

  return true;
}

// The alpha texture an instance overrides its mesh with. Only the bindless index survives in the meta,
// thus the id comes back from the allocator that handed it out.
static TEXTUREID get_override_alpha_texture_id(ContextId context_id, const MeshMeta &meta)
{
  if (meta.alphaTextureIndex == MeshMeta::INVALID_TEXTURE)
    return BAD_TEXTUREID;

  WinAutoLock lock(context_id->bindlessTextureLock);
  auto texture = context_id->bindlessTextureAllocator.get_resource(meta.alphaTextureIndex);
  return texture ? texture->getTID() : BAD_TEXTUREID;
}

// The same state walk as the mesh-shared bake, on an entry that no Mesh owns; thus the terminal states
// publish to the debug viewer from here, as no build or consume path has a mesh to publish under.
static OverrideOmmResult advance_override_omm_entry(ContextId context_id, uint64_t object_id, const Mesh &mesh, OmmCacheEntry &entry,
  const OmmBakeSource &source, TEXTUREID override_tex_id, uint32_t alpha_channel, bool in_delayed_sync_window)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  entry.lastPollFrame = dagor_frame_no();

  const auto publishOnce = [&] {
    if (entry.debugPublished)
      return;
    entry.debugPublished = true;
    publish_omm_debug_result(mesh, entry, object_id, mesh.firstGeometryIndex, OMM_PRIMARY_SLOT, in_delayed_sync_window,
      override_tex_id);
  };

  switch (entry.state)
  {
    case OmmState::Built: publishOnce(); return OverrideOmmResult::UseEntry;
    case OmmState::Failed:
      // Failed but absorbed: the instance takes the result as geometry flags in place of dropping out of
      // the BVH, and an absorbed result is not shown, as the mesh path.
      if (omm_entry_should_be_opaque(&entry) || omm_entry_should_be_skipped(&entry))
      {
        // The build kept these buffers for a later publish that the absorb never makes. The resolve runs
        // for every instance in every frame, thus release them on the first one.
        if (entry.bakeResult.arrayData)
          release_omm_result(context_id->ommContext, entry.bakeResult);
        return OverrideOmmResult::UseEntry;
      }
      publishOnce();
      // The per-frame resolve keeps this entry alive, thus the eviction never gives it a fresh start;
      // retry a transient failure on its own interval instead.
      if (!is_transient_omm_failure(entry.failure) || dagor_frame_no() - entry.failFrame <= OMM_OVERRIDE_RETRY_FRAMES)
        return OverrideOmmResult::WithholdAndReportFailure;
      entry.resetBakeState();
      break;
    // An ownerless bake still reaches a terminal state through the baking list.
    case OmmState::Baking:
    case OmmState::Ready: return OverrideOmmResult::Withhold;
    case OmmState::None: break;
  }

  return advance_unbaked_omm_entry(context_id, object_id, mesh, entry, source, OMM_PRIMARY_SLOT, override_tex_id, alpha_channel,
           in_delayed_sync_window) == OmmBakeAdvance::Failed
           ? OverrideOmmResult::WithholdAndReportFailure
           : OverrideOmmResult::Withhold;
}

OverrideOmmResult resolve_override_omm_entry(ContextId context_id, uint64_t object_id, const Mesh &mesh, const MeshMeta &meta,
  const OmmBakeSource &source, bool in_delayed_sync_window, OmmCacheEntry *&out_entry, TEXTUREID &out_override_tex_id)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  out_entry = nullptr;
  out_override_tex_id = BAD_TEXTUREID;

  // An impostor bakes from its albedo texture, thus an alpha override says nothing about its bake source.
  if (mesh.materialType & MeshMeta::bvhMaterialImpostor)
    return OverrideOmmResult::WithholdAndReportOverride;

  const TEXTUREID overrideTexId = get_override_alpha_texture_id(context_id, meta);
  if (overrideTexId == BAD_TEXTUREID)
    return OverrideOmmResult::WithholdAndReportOverride;
  out_override_tex_id = overrideTexId;

  // A bake source no bake can read is a property of the mesh, thus reporting it as an override problem
  // sends the reader after the wrong cause.
  if (!is_valid_omm_bake_source(source))
    return OverrideOmmResult::WithholdAndReportBakeSource;

  // The ref goes on return, and its drop restamps the idle age: an entry an instance keeps resolving
  // cannot age out while it bakes. The blas that links the entry is what holds it past this scope.
  OmmEntryRef ref = acquire_omm_entry(context_id, make_omm_cache_key(mesh, object_id, OMM_PRIMARY_SLOT, overrideTexId));
  const OverrideOmmResult result = advance_override_omm_entry(context_id, object_id, mesh, *ref, source, overrideTexId,
    omm_alpha_channel(meta.materialType), in_delayed_sync_window);
  if (result == OverrideOmmResult::UseEntry || result == OverrideOmmResult::WithholdAndReportFailure)
    out_entry = ref.get();
  return result;
}

static void consume_mesh_omm_slot(ContextId context_id, Mesh &mesh, int slot_id, OmmBuildInfos &build_infos,
  OmmBuildResults &build_results, bool in_delayed_sync_window)
{
  OmmCacheEntry *entry = mesh.ommEntries[slot_id].get();
  if (!entry)
    return;

  const bool wasBaking = entry->state == OmmState::Baking;
  entry->lastPollFrame = dagor_frame_no();
  poll_baking_omm_entry(context_id, *entry);
  // A failed readback is never Ready, thus the build step cannot publish it.
  if (wasBaking && entry->state == OmmState::Failed)
    publish_omm_debug_result(mesh, *entry, entry->bakeObjectId, mesh.firstGeometryIndex, slot_id, in_delayed_sync_window);
  build_omm_if_ready(context_id, *entry, &mesh, slot_id, build_infos, build_results, in_delayed_sync_window);
}

void consume_mesh_omm_bakes(ContextId context_id, Mesh &mesh, OmmBuildInfos &build_infos, OmmBuildResults &build_results,
  bool in_delayed_sync_window)
{
  consume_mesh_omm_slot(context_id, mesh, OMM_PRIMARY_SLOT, build_infos, build_results, in_delayed_sync_window);
  if (needs_secondary_omm(mesh))
    consume_mesh_omm_slot(context_id, mesh, OMM_SECONDARY_SLOT, build_infos, build_results, in_delayed_sync_window);
}

// Diagnostics only: mesh is null while the object that started the bake is torn down.
struct OmmEntryOwner
{
  Mesh *mesh = nullptr;
  int slotId = OMM_PRIMARY_SLOT;
};

static OmmEntryOwner find_omm_entry_owner(ContextId context_id, const OmmCacheEntry *entry)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  Object *object = find_half_baked_object(context_id, entry->bakeObjectId);
  if (!object)
    return {};

  for (Mesh &mesh : object->meshes)
    for (int slotId = OMM_PRIMARY_SLOT; slotId <= OMM_SECONDARY_SLOT; ++slotId)
      if (mesh.ommEntries[slotId].get() == entry)
        return {&mesh, slotId};
  return {};
}

void consume_ready_omm_bakes(ContextId context_id, OmmBuildInfos &build_infos, OmmBuildResults &build_results)
{
  if (!context_id->ommEnabled)
    return;

  TIME_PROFILE(consume_ready_omm_bakes);

  dag::Vector<OmmCacheEntry *, framemem_allocator> entries(context_id->bakingOmmEntries.begin(), context_id->bakingOmmEntries.end());
  for (OmmCacheEntry *entry : entries)
  {
    poll_baking_omm_entry(context_id, *entry);
    if (entry->state != OmmState::Ready && entry->state != OmmState::Failed)
      continue;

    const OmmEntryOwner owner = find_omm_entry_owner(context_id, entry);
    if (entry->state == OmmState::Failed)
    {
      if (owner.mesh)
        publish_omm_debug_result(*owner.mesh, *entry, entry->bakeObjectId, owner.mesh->firstGeometryIndex, owner.slotId, false);
      continue;
    }

    // Runs outside of any delayed-sync window.
    build_omm_if_ready(context_id, *entry, owner.mesh, owner.slotId, build_infos, build_results, false);
  }
}

OmmCacheEntry *linkable_omm_entry(OmmCacheEntry *entry)
{
  return entry && entry->state == OmmState::Built && entry->omm ? entry : nullptr;
}

OmmCacheEntry *linkable_omm_entry(const Mesh &mesh, int slot_id) { return linkable_omm_entry(mesh.ommEntries[slot_id].get()); }

void set_omm_linkage(RaytraceGeometryDescription &desc, OmmCacheEntry *entry)
{
  entry = linkable_omm_entry(entry);
  if (!entry)
    return;

  desc.ommLinkage = render::omm::make_geometry_linkage(entry->bakeResult, entry->omm.get());
  desc.extraDataAvailableMask.hasOpacityMicroMapLinkage = true;
}

void set_omm_linkage(RaytraceGeometryDescription &desc, Mesh &mesh, int slot_id)
{
  set_omm_linkage(desc, mesh.ommEntries[slot_id].get());
}

bool mesh_omms_built(const Mesh &mesh)
{
  return linkable_omm_entry(mesh, OMM_PRIMARY_SLOT) && (!needs_secondary_omm(mesh) || linkable_omm_entry(mesh, OMM_SECONDARY_SLOT));
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

bool omm_entry_should_be_opaque(const OmmCacheEntry *entry)
{
  return !bvh_strict_asset_checks && entry && entry->failure == OmmFailure::AllTrianglesOpaque;
}

bool omm_entry_should_be_skipped(const OmmCacheEntry *entry)
{
  return !bvh_strict_asset_checks && entry && entry->failure == OmmFailure::AllTrianglesTransparent;
}

bool mesh_should_be_opaque(const Mesh &mesh)
{
  // All slots must agree: the flag applies to the full mesh, thus secondary geometry with a cutout blocks
  // this.
  return omm_entry_should_be_opaque(mesh.ommEntries[OMM_PRIMARY_SLOT].get()) &&
         (!needs_secondary_omm(mesh) || omm_entry_should_be_opaque(mesh.ommEntries[OMM_SECONDARY_SLOT].get()));
}

bool mesh_should_be_skipped(const Mesh &mesh)
{
  return omm_entry_should_be_skipped(mesh.ommEntries[OMM_PRIMARY_SLOT].get()) &&
         (!needs_secondary_omm(mesh) || omm_entry_should_be_skipped(mesh.ommEntries[OMM_SECONDARY_SLOT].get()));
}

void make_mesh_opaque(Mesh &mesh, MeshMeta &base_meta)
{
  // mesh.materialType is what matters: the BLAS build reads it to mark the geometry IS_OPAQUE, and
  // mesh_wants_omm becomes false. The meta copy is cleared only to keep the two from disagreeing.
  mesh.materialType &= ~MeshMeta::bvhMaterialAlphaTest;
  base_meta.materialType &= ~MeshMeta::bvhMaterialAlphaTest;
}

void fail_omm_entry(ContextId context_id, OmmCacheEntry &entry, OmmFailure failure)
{
  if (entry.state == OmmState::Baking)
    abort_omm_entry_bake(context_id, entry);
  entry.state = OmmState::Failed;
  entry.failure = failure;
  entry.failFrame = dagor_frame_no();
}

const char *omm_failure_text(OmmFailure failure)
{
  switch (failure)
  {
    case OmmFailure::None: return "no reason was recorded";
    case OmmFailure::UnsupportedTexcoordFormat: return "its texcoord packing is not one the OMM bake can decode";
    case OmmFailure::NoAlphaSource: return "no alpha or albedo texture is available to bake an OMM from";
    case OmmFailure::AlphaTextureNeverLoaded:
      return "its alpha source never reached full quality within the wait budget, so no OMM could be baked from it";
    case OmmFailure::BakeStartFailed: return "the bake could not be started (alpha texture acquire or texcoord setup failed)";
    case OmmFailure::ReadbackInvalid: return "the GPU bake readback returned no valid data";
    case OmmFailure::NoOutputBuffers: return "the bake produced no output buffers";
    case OmmFailure::NoDescriptors:
      return "the bake produced no OMM descriptors (every micro-triangle collapsed to a single state, and not all to opaque or all "
             "to transparent)";
    case OmmFailure::AllTrianglesOpaque:
      return "its alpha source is fully opaque over the whole mesh, so the material should not be alpha-tested at all";
    case OmmFailure::AllTrianglesTransparent:
      return "its alpha source is fully transparent over the whole mesh, so the raster draws none of it: delete this dead geometry";
    case OmmFailure::ZeroArraySize: return "the OMM array reported a zero acceleration-structure size";
    case OmmFailure::InstanceAlphaSourceOverride:
      return "this instance alpha-tests against a texture of its own that no OMM can be baked from: an impostor bakes from its "
             "albedo, and an override texture must resolve to a texture id";
  }
  return "unrecognized failure";
}

// The shared texts tell the artist to remove the alpha test on a fully opaque bake, and to delete a
// fully transparent mesh. Grass is always a cutout and can do neither, thus both results mean that
// its alpha source is bad.
const char *grass_omm_failure_text(OmmFailure failure)
{
  if (failure == OmmFailure::AllTrianglesOpaque)
    return "its alpha source came back fully opaque, and grass is always a cutout -- the alpha texture is wrong";
  if (failure == OmmFailure::AllTrianglesTransparent)
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

static String describe_omm_bake_attempt(const OmmCacheEntry *entry)
{
  if (!entry)
    return String("; no bake was started");

  if (entry->bakeSubdivisionLevel == OmmCacheEntry::NO_BAKE_STARTED)
    return entry->bakeTexcoordFormat
             ? String(0, "; no bake was started, texcoords %s", bvh_texcoord_format_desc(entry->bakeTexcoordFormat).c_str())
             : String("; no bake was started");

  String desc(0, "; baked texcoords %s at a maximum subdivision level of %u%s",
    bvh_texcoord_format_desc(entry->bakeTexcoordFormat).c_str(), entry->bakeSubdivisionLevel,
    entry->bakeUvCutout ? " through a texcoord cutout" : "");
  if (entry->bakeSubdivisionLevel == 0)
    desc += " (the data array budget allowed no subdivision, so every triangle is a single micro-triangle "
            "and can only resolve to a uniform state)";

  // All counts zero means no bake completed, not that the bake found nothing.
  const render::omm::BakeStats &s = entry->bakeStats;
  if (s.totalOpaqueCount || s.totalTransparentCount || s.totalUnknownCount || s.totalFullyOpaqueCount ||
      s.totalFullyTransparentCount || s.totalFullyUnknownCount)
    desc.aprintf(0,
      " [triangles fully-opaque=%u fully-transparent=%u fully-unknown=%u; micro-triangles opaque=%u transparent=%u unknown=%u]",
      s.totalFullyOpaqueCount, s.totalFullyTransparentCount, s.totalFullyUnknownCount, s.totalOpaqueCount, s.totalTransparentCount,
      s.totalUnknownCount);
  return desc;
}

String describe_missing_omm(ContextId context_id, const Mesh &mesh, const OmmBakeSource &source)
{
  const TEXTUREID texId = get_omm_texture_id(mesh);
  const char *texName = texId != BAD_TEXTUREID ? get_managed_texture_name(texId) : nullptr;
  const String texDesc(0, "texture '%s'", texName ? texName : "<none>");

  if (!mesh_wants_omm(context_id, mesh))
    return String(0, "%s", omm_failure_text(OmmFailure::NoAlphaSource));

  if (!source.texCoordBuffer)
    return String(0,
      "the OMM bake found no texcoords to read (the mesh has no texcoord channel, or an impostor's vertex "
      "processing never ran); %s",
      texDesc.c_str());

  render::omm::TexCoordFormat unusedFormat;
  if (!get_omm_texcoord_format(source.texCoordFormat, unusedFormat))
    return String(0, "%s (%s); %s", omm_failure_text(OmmFailure::UnsupportedTexcoordFormat),
      bvh_texcoord_format_desc(source.texCoordFormat).c_str(), texDesc.c_str());

  String failures;
  for (int slotId = OMM_PRIMARY_SLOT; slotId <= OMM_SECONDARY_SLOT; ++slotId)
  {
    const OmmCacheEntry *entry = mesh.ommEntries[slotId].get();
    if (!entry || entry->state != OmmState::Failed)
      continue;
    failures.aprintf(0, "%s%s%s%s", failures.empty() ? "" : " / ", slotId == OMM_SECONDARY_SLOT ? "secondary geometry: " : "",
      omm_failure_text(entry->failure), describe_omm_bake_attempt(entry).c_str());
  }
  if (!failures.empty())
    return String(0, "%s; %s", failures.c_str(), texDesc.c_str());

  return String(0, "the bake has not produced a usable result; %s%s", texDesc.c_str(),
    describe_omm_bake_attempt(mesh.ommEntries[OMM_PRIMARY_SLOT].get()).c_str());
}

void discard_inactive_omm_bakes(ContextId context_id)
{
  if (!context_id->ommEnabled)
    return;

  // The grace window lets a dead object's in-flight bake finish and be adopted by a quick re-add; past
  // it the slot is taken back.
  auto &entries = context_id->bakingOmmEntries;
  for (size_t i = 0; i < entries.size();)
  {
    OmmCacheEntry &entry = *entries[i];
    if (!omm_bake_is_inactive(context_id, entry, OMM_BAKE_GIVE_UP_FRAMES))
    {
      ++i;
      continue;
    }
    discard_omm_entry_bake(context_id, entry);
  }
}

// The release also removes the debug-viewer registration that a publish may have made.
// Never reached with a bake in flight: both eviction paths skip those.
static OmmCache::iterator evict_omm_entry(ContextId context_id, OmmCache::iterator iter)
{
  release_omm_result(context_id->ommContext, iter->second.bakeResult);
  return context_id->ommCache.erase(iter);
}

void evict_idle_omm_entries(ContextId context_id)
{
  if (!context_id->ommEnabled)
    return;

  drain_deferred_omm_releases(context_id);

  const uint32_t frame = dagor_frame_no();
  // Recomputed, not tracked on the refcount edges: an entry's byte count still changes while it is idle.
  uint64_t idleBytes = 0;
  dag::Vector<OmmCache::iterator, framemem_allocator> idleEntries;
  for (auto iter = context_id->ommCache.begin(); iter != context_id->ommCache.end();)
  {
    const OmmCacheEntry &entry = iter->second;
    // A recent poll means the entry is in use whatever the refcount says: a same-frame re-add drops its
    // refs before the resolve takes them again, and an override entry holds none between its resolves.
    if (entry.refCount || frame - entry.lastPollFrame <= 1)
    {
      ++iter;
      continue;
    }
    // An in-flight bake is never evicted, by age or by budget: it always reaches a terminal state
    // through the baking list, and dropping it only makes the next resolve start the same bake again.
    const bool baking = entry.state == OmmState::Baking;
    if (!baking && frame - entry.zeroRefFrame > bvh_omm_cache_retention_frames)
    {
      iter = evict_omm_entry(context_id, iter);
      continue;
    }
    // Only the evictable bytes drive the budget, thus what it counts is what a pass over it can free.
    if (!baking)
    {
      idleBytes += omm_entry_bytes(entry);
      idleEntries.push_back(iter);
    }
    ++iter;
  }

  if (idleBytes > bvh_omm_cache_idle_budget)
  {
    eastl::sort(idleEntries.begin(), idleEntries.end(),
      [](const OmmCache::iterator &a, const OmmCache::iterator &b) { return a->second.zeroRefFrame < b->second.zeroRefFrame; });
    for (const OmmCache::iterator &iter : idleEntries)
    {
      if (idleBytes <= bvh_omm_cache_idle_budget)
        break;
      idleBytes -= omm_entry_bytes(iter->second);
      evict_omm_entry(context_id, iter);
    }
  }
}

void release_omm_bake_build_inputs(render::omm::Context &omm_ctx, OmmBuildResults &omm_build_results)
{
  // The viewer draws from these buffers, thus with it on they stay with the result until the owner of
  // the result releases it.
  if (bvh_retain_omm_bake_results)
  {
    omm_build_results.clear();
    return;
  }

  for (render::omm::BakeResult *result : omm_build_results)
    if (result)
      render::omm::recycle_result(omm_ctx, *result, /*keep_index_buffer*/ true);
  omm_build_results.clear();
}

OmmFailure build_grass_omm_array(render::omm::BakeResult &result, const render::omm::BakeStats &stats, UniqueOMM &out_omm,
  OmmBuildInfos &build_infos, OmmBuildResults &build_results)
{
  if (!result.arrayData || !result.descArray || !result.indexBuffer)
    return OmmFailure::NoOutputBuffers;
  if (result.arrayBuildDescs.empty() || result.blasLinkageDescs.empty())
    return bake_is_all_opaque(stats)        ? OmmFailure::AllTrianglesOpaque
           : bake_is_all_transparent(stats) ? OmmFailure::AllTrianglesTransparent
                                            : OmmFailure::NoDescriptors;

  auto sizeInfo = render::omm::make_array_build_info(result, nullptr, 0, 0, RaytraceBuildFlags::FAST_TRACE);
  const raytrace::AccelerationStructureSizes sizes = d3d::raytrace::calculate_acceleration_structure_sizes(sizeInfo);
  if (!sizes.structureSizeInBytes)
    return OmmFailure::ZeroArraySize;

  out_omm = UniqueOMM::create_omm(sizes.structureSizeInBytes);
  HANDLE_LOST_DEVICE_STATE(out_omm, OmmFailure::None);

  uint32_t scratchOffset = 0;
  Sbuffer *scratchBuffer = alloc_scratch_buffer(sizes.buildScratchBufferSizeInBytes, scratchOffset);
  if (sizes.buildScratchBufferSizeInBytes)
    HANDLE_LOST_DEVICE_STATE(scratchBuffer, OmmFailure::None);

  raytrace::BatchedOpacityMicroMapTriangleArrayBuildInfo build;
  build.omm = out_omm.get();
  build.ommtabi = render::omm::make_array_build_info(result, scratchBuffer, scratchOffset, sizes.buildScratchBufferSizeInBytes,
    RaytraceBuildFlags::FAST_TRACE);
  build_infos.push_back(build);
  build_results.push_back(&result);
  return OmmFailure::None;
}

void build_pending_omm_arrays(render::omm::Context &omm_ctx, OmmBuildInfos &omm_builds, OmmBuildResults &omm_build_results)
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
  release_omm_bake_build_inputs(omm_ctx, omm_build_results);
}

} // namespace bvh
