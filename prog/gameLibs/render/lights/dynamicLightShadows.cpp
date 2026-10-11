// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/dynamicLightShadows.h>
#include <render/lights/shadowSystem.h>
#include <render/lights/lightsVisibilityChecker.h>
#include <render/lights/renderLights.hlsli>
#include <render/depthUtil.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_barrier.h>
#include <memory/dag_framemem.h>
#include <generic/dag_staticTab.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/type_traits.h>
#include <debug/dag_debug.h>

static const char *in_frustum_common_light_shadows_buffer_name(LightBufferSlot slot)
{
  return slot != LightBufferSlot::Main ? "common_lights_shadows_secondary" : "common_lights_shadows";
}

static const char *out_of_frustum_common_light_shadows_buffer_name = "out_of_frustum_common_lights_shadow_data";
static const char *ssss_shadow_desc_buffer_name = "spot_lights_ssss_shadow_desc";

#define DYNAMIC_LIGHT_SHADOWS_GLOBAL_VARS_LIST \
  VAR(scene_omni_managed_lights)               \
  VAR(scene_spot_managed_lights)               \
  VAR(visible_lights_data_buf)                 \
  VAR(visible_lights_counts_buf)               \
  VAR(dynamic_lights_shadows_data_buf)         \
  VAR(dynamic_lights_shadows_transit_buf)      \
  VAR(spot_lights_ssss_shadow_desc)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
DYNAMIC_LIGHT_SHADOWS_GLOBAL_VARS_LIST
#undef VAR
#undef DYNAMIC_LIGHT_SHADOWS_GLOBAL_VARS_LIST

DynamicLightShadows::DynamicLightShadows(const LightsResourcesManager *lights_res_mgr, OmniLightsManager *omni_lights_manager,
  SpotLightsManager *spot_lights_manager) :
  omniLightsManager(omni_lights_manager), spotLightsManager(spot_lights_manager), lightsResMgr(lights_res_mgr)
{}

static_assert(sizeof(SpotlightShadowDescriptor) == SHADOW_SLOT_SSSS_DESC_POINT4_SIZE * sizeof(Point4));

static bool has_actual_shadow_data(const ShadowSystem *shadow_system, uint16_t shadow_id)
{
  return shadow_id != INVALID_SHADOW_VOLUME_ID && shadow_system && shadow_system->hasVolumeEverBeenRendered(shadow_id);
}

static void write_spot_light_shadow_data(const ShadowSystem *shadow_system, uint16_t shadow_id, Point4 *dst)
{
  if (has_actual_shadow_data(shadow_system, shadow_id))
  {
    const auto &volumeTexMat = shadow_system->getVolumeTexMatrix(shadow_id);
    static_assert(sizeof(volumeTexMat) == sizeof(float4x4), "ShadowSystem tex matrix is copied as float4x4");
    memcpy(dst, &volumeTexMat, sizeof(float4x4));
    dst[SPOT_LIGHT_SHADOW_UV_BOUNDS_POINT4_OFFSET] = shadow_system->getShadowUvMinMax(shadow_id);
  }
  else
  {
    const auto defaultSpotVolumeTexMatrix = DLS_DEFAULT_SPOT_LIGHT_TM;
    memcpy(dst, &defaultSpotVolumeTexMatrix, sizeof(float4x4));
    dst[SPOT_LIGHT_SHADOW_UV_BOUNDS_POINT4_OFFSET] = DLS_DEFAULT_SPOT_LIGHT_SHADOW_UV_BOUNDS;
  }
}

static Point4 get_omni_light_shadow_data(const ShadowSystem *shadow_system, uint16_t shadow_id)
{
  return has_actual_shadow_data(shadow_system, shadow_id) ? shadow_system->getOctahedralVolumeTexData(shadow_id)
                                                          : DLS_DEFAULT_OMNI_LIGHT_SHADOW_DATA;
}

static SpotlightShadowDescriptor get_spot_light_ssss_descriptor(const ShadowSystem *shadow_system, uint16_t shadow_id)
{
  SpotlightShadowDescriptor shadowDesc;
  memset(&shadowDesc, 0, sizeof(shadowDesc));
  if (!has_actual_shadow_data(shadow_system, shadow_id))
    return shadowDesc;

  float wk;
  Point2 zn_zfar;
  shadow_system->getVolumeInfo(shadow_id, wk, zn_zfar.x, zn_zfar.y);
  shadowDesc.decodeDepth = get_decode_depth(zn_zfar);

  Point2 shadowUvSize = shadow_system->getShadowUvSize(shadow_id);
  shadowDesc.meterToUvAtZfar = max(shadowUvSize.x, shadowUvSize.y) / (2 * wk);
  shadowDesc.uvMinMax = shadow_system->getShadowUvMinMax(shadow_id);

  bool hintDynamic;
  ShadowCastersFlag casters;
  uint16_t quality;
  uint8_t priority, size;
  DynamicShadowRenderGPUObjects renderGPUObjects;
  shadow_system->getShadowProperties(shadow_id, casters, hintDynamic, quality, priority, size, renderGPUObjects);
  shadowDesc.hasDynamic = static_cast<float>((casters & ShadowCastersFlag::Dynamic) != ShadowCastersFlag::None);
  return shadowDesc;
}

static void write_shadow_slot(const ShadowSystem *shadow_system, uint16_t shadow_id, Point4 *slot)
{
  if (shadow_system->isVolumeOctahedral(shadow_id))
  {
    slot[0] = get_omni_light_shadow_data(shadow_system, shadow_id);
    memset(&slot[1], 0, (SHADOW_SLOT_POINT4_STRIDE - 1) * sizeof(Point4));
    return;
  }
  write_spot_light_shadow_data(shadow_system, shadow_id, slot);
  const SpotlightShadowDescriptor shadowDesc = get_spot_light_ssss_descriptor(shadow_system, shadow_id);
  memcpy(&slot[SHADOW_SLOT_SSSS_DESC_POINT4_OFFSET], &shadowDesc, sizeof(shadowDesc));
}

void DynamicLightShadows::init(DynLightsFeatures requested_features)
{
  shadowDataBufferInitialized = false;
  shadowDataBuffer.close();
  shadowsTransitBuffer.close();
  gatherCS = static_cast<bool>(requested_features & DynLightsFeatures::GpuLightShadowsPersistentStorage)
               ? ComputeShader("dynamic_lights_shadows_gather", true)
               : ComputeShader();
  if (gatherCS)
  {
    shadowDataBuffer = dag::buffers::create_persistent_sr_byte_address(
      DYNAMIC_LIGHTS_SHADOW_DATA_POINT4_SIZE * sizeof(Point4) / d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE,
      lightsResMgr->getResName("dynamic_lights_shadows_data_buf"), d3d::buffers::Init::No, RESTAG_LIGHTS);
    shadowsTransitBuffer = dag::buffers::create_ua_sr_byte_address(
      COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE * sizeof(Point4) / d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE,
      lightsResMgr->getResName("dynamic_lights_shadows_transit_buf"), d3d::buffers::Init::No, RESTAG_LIGHTS);
  }
  recreateSpotLightSsssShadowDescBuffer();
}

bool DynamicLightShadows::resetShadowDataBufferIfNecessary()
{
  G_ASSERT(isGpu());
  if (shadowDataBufferInitialized)
    return true;

  Tab<Point4> data;
  data.resize(DYNAMIC_LIGHTS_SHADOW_DATA_POINT4_SIZE);
  const float4x4 defaultSpotTm = DLS_DEFAULT_SPOT_LIGHT_TM;
  for (uint32_t slot = 0; slot < MAX_DYNAMIC_LIGHTS_SHADOW_SLOTS; ++slot)
  {
    const uint32_t slotOffset = slot * SHADOW_SLOT_POINT4_STRIDE;
    memcpy(&data[slotOffset], &defaultSpotTm, sizeof(defaultSpotTm));
    data[slotOffset + SPOT_LIGHT_SHADOW_UV_BOUNDS_POINT4_OFFSET] = DLS_DEFAULT_SPOT_LIGHT_SHADOW_UV_BOUNDS;
    memset(&data[slotOffset + SHADOW_SLOT_SSSS_DESC_POINT4_OFFSET], 0, sizeof(SpotlightShadowDescriptor));
  }
  if (!shadowDataBuffer->updateData(0, data_size(data), data.data(), VBLOCK_WRITEONLY))
    return false;

  if (shadowSystem)
    shadowSystem->markAllShadowsUpdated();
  shadowDataBufferInitialized = true;
  return true;
}

DynLightsFeatures DynamicLightShadows::getRuntimeFeatures() const
{
  return (static_cast<bool>(shadowDataBuffer) && static_cast<bool>(shadowsTransitBuffer))
           ? DynLightsFeatures::GpuLightShadowsPersistentStorage
           : DynLightsFeatures::CpuOnly;
}

void DynamicLightShadows::close()
{
  shadowDataBuffer.close();
  shadowDataBufferInitialized = false;
  shadowsTransitBuffer.close();
  gatherCS = ComputeShader();

  spotLightSsssShadowDescBuffer.close();
  for (auto &cb : commonLightShadowsBufferCB)
    cb.close();
  outOfFrustumCommonLightShadowsCB.close();
  outOfFrustumCommonLightShadowsAreEmpty = false;
}

void DynamicLightShadows::setShadowSystem(ShadowSystem *shadow_system)
{
  if (shadowSystem != shadow_system)
    shadowDataBufferInitialized = false;
  shadowSystem = shadow_system;
}

void DynamicLightShadows::resetShadowVolumeUpdateFlag(uint16_t shadow_id) { dynamicLightsShadowsVolumeSet.reset(shadow_id); }

void DynamicLightShadows::resetShadowVolumeUpdateFlags() { dynamicLightsShadowsVolumeSet.reset(); }

bool DynamicLightShadows::lazyInitCommonLightShadowsCB(CommonLightShadowsCBType &cb, const char *buffer_name) const
{
  if (cb.getBuf())
    return false;
  const Tab<uint16_t> noLights;
  updateCommonLightShadowsConstBufferCPU(cb, buffer_name, noLights, noLights, true);
  return true;
}

const DynamicLightShadows::CommonLightShadowsCBType &DynamicLightShadows::getInFrustumCommonLightShadowsCB(LightBufferSlot slot) const
{
  const LightBufferSlot bufferSlot = isGpu() ? LightBufferSlot::Main : slot;
  auto &cb = commonLightShadowsBufferCB[size_t(bufferSlot)];
  lazyInitCommonLightShadowsCB(cb, in_frustum_common_light_shadows_buffer_name(bufferSlot));
  return cb;
}

const DynamicLightShadows::CommonLightShadowsCBType &DynamicLightShadows::getOutOfFrustumCommonLightShadowsCB() const
{
  if (lazyInitCommonLightShadowsCB(outOfFrustumCommonLightShadowsCB, out_of_frustum_common_light_shadows_buffer_name))
    outOfFrustumCommonLightShadowsAreEmpty = true;
  return outOfFrustumCommonLightShadowsCB;
}

void DynamicLightShadows::setLightShadowVolume(BaseLightsManager *lights_manager, int light_id)
{
  uint32_t shadowId = lights_manager->getShadowId(light_id);
  if (shadowId == INVALID_SHADOW_VOLUME_ID || dynamicLightsShadowsVolumeSet.test(shadowId))
    return;

  lights_manager->updateShadowVolume(light_id);
  dynamicLightsShadowsVolumeSet.set(shadowId);
}

void DynamicLightShadows::framePrepareShadows(dynamic_shadow_render::FrameVolumeData &volume_data,
  const dynamic_shadow_render::FramePrepareShadowsParams &params)
{
  volume_data.clear();
  if (!shadowSystem || !params.lightVisibilityChecker)
    return;
  TIME_PROFILE(prepare_spot_omni_shadows);

  shadowSystem->startPrepareShadows();

  const auto shadowedLightCallback = [this, &params](BaseLightsManager *lights_manager, LightType light_type, uint32_t light_id) {
    if (params.lightVisibilityChecker->test(light_type, light_id) != LightsVisibilityChecker::TestResult::Clustered)
      return;
    setLightShadowVolume(lights_manager, light_id);
    shadowSystem->useShadowOnFrame(lights_manager->getShadowId(light_id));
  };

  spotLightsManager->forEachLightsWithCloseShadow(params.viewPos, maxShadowDist,
    [&](uint32_t light_id) { shadowedLightCallback(spotLightsManager, LightType::Spot, light_id); });

  omniLightsManager->forEachLightsWithCloseShadow(params.viewPos, maxShadowDist,
    [&](uint32_t light_id) { shadowedLightCallback(omniLightsManager, LightType::Omni, light_id); });

  shadowSystem->setDynamicObjectsContent(params.dynamicBoxes.data(), params.dynamicBoxes.size()); // dynamic content within those boxes

  float maxAreaToUpdate = max((float)maxShadowsToUpdateOnFrame / DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME, 1.0f) * 0.25f;
  shadowSystem->endPrepareShadows(volume_data, maxShadowsToUpdateOnFrame, maxShadowViewsToUpdateOnFrame, maxAreaToUpdate,
    params.viewPos, params.cameraFocal, params.globtm);

  if (!params.collectUpdates)
    return;

  for (int i = volume_data.volumes.size() - 1; i >= 0; --i)
  {
    auto &vol = volume_data.volumes[i];
    const auto renderFlags = shadowSystem->getVolumeRenderFlags(vol.id);
    if (renderFlags & ShadowSystem::RENDER_STATIC)
    {
      vol.staticUpdateIndex = volume_data.staticUpdates.size();
      shadowSystem->getVolumeUpdateData(vol.id, volume_data.staticUpdates.emplace_back());
    }
    if (renderFlags & ShadowSystem::RENDER_DYNAMIC)
    {
      vol.dynamicUpdateIndex = volume_data.dynamicUpdates.size();
      shadowSystem->getVolumeUpdateData(vol.id, volume_data.dynamicUpdates.emplace_back());
    }
  }
}

void DynamicLightShadows::frameRenderShadows(const dynamic_shadow_render::FrameVolumeData &volume_data,
  StaticRenderCallback renderStatic, DynamicRenderCallback renderDynamic, shaders::OverrideStateId depth_bias_override_id,
  shaders::OverrideStateId depth_bias_two_sided_override_id)
{
  if (!shadowSystem)
    return;

  shadowSystem->setTextureToShader();
  if (volume_data.volumes.empty())
    return;

  shadowSystem->startRenderVolumes(volume_data);
  bool staticOverrideState = false;
  shaders::OverrideStateId originalState = shaders::overrides::get_current();
  for (int i = volume_data.volumes.size() - 1; i >= 0; --i)
  {
    const auto &vol = volume_data.volumes[i];
    shaders::overrides::set(depth_bias_override_id);
    mat44f view, proj, viewItm;
    const int id = vol.id;
    ShadowSystem::RenderFlags renderFlags;
    uint32_t numViews = shadowSystem->startRenderVolume(id, proj, renderFlags);
    if (renderFlags & ShadowSystem::RENDER_STATIC)
    {
      TIME_D3D_PROFILE(staticShadow);
      if (!staticOverrideState)
      {
        shaders::overrides::reset();
        shaders::overrides::set(depth_bias_override_id);
        staticOverrideState = true;
      }

      for (uint32_t viewId = 0; viewId < numViews; ++viewId)
      {
        shadowSystem->startRenderVolumeView(id, viewId, viewItm, view, renderFlags, ShadowSystem::RENDER_STATIC);
        alignas(16) TMatrix viewItmS;
        v_mat_43ca_from_mat44(viewItmS[0], viewItm);

        d3d::settm(TM_VIEW, view);
        d3d::settm(TM_PROJ, proj);
        mat44f globTm;
        v_mat44_mul(globTm, proj, view);

        bool hint_dynamic;
        ShadowCastersFlag casters;
        uint8_t priority, shadow_size_srl;
        uint16_t quality;
        DynamicShadowRenderGPUObjects render_gpu_objects;
        shadowSystem->getShadowProperties(id, casters, hint_dynamic, quality, priority, shadow_size_srl, render_gpu_objects);

        renderStatic(globTm, proj, viewItmS, vol.staticUpdateIndex, viewId, render_gpu_objects);
        shadowSystem->endRenderVolumeView(id, viewId);
      }
      shadowSystem->endRenderStatic(id);
    }
    if (renderFlags & ShadowSystem::RENDER_DYNAMIC)
    {
      TIME_D3D_PROFILE(dynamicShadow);
      staticOverrideState = false;
      shaders::overrides::reset(); // startRenderDynamic uses an other state
      shadowSystem->startRenderDynamic(id);
      shaders::overrides::set(shadowSystem->isShadowTwoSided(id) ? depth_bias_two_sided_override_id : depth_bias_override_id);
      for (uint32_t viewId = 0; viewId < numViews; ++viewId)
      {
        shadowSystem->startRenderVolumeView(id, viewId, viewItm, view, renderFlags, ShadowSystem::RENDER_DYNAMIC);
        alignas(16) TMatrix viewItmS;
        v_mat_43ca_from_mat44(viewItmS[0], viewItm);

        d3d::settm(TM_VIEW, view);
        d3d::settm(TM_PROJ, proj);

        renderDynamic(viewItmS, view, proj, vol.dynamicUpdateIndex, viewId);
        shadowSystem->endRenderVolumeView(id, viewId);
      }
      shaders::overrides::reset();
    }
    shadowSystem->endRenderVolume(id);
    shaders::overrides::reset();
  }
  shaders::overrides::reset();
  shaders::overrides::set(originalState);
  shadowSystem->endRenderVolumes();
}

void DynamicLightShadows::syncShadowDataGPU()
{
  G_ASSERT_RETURN(isGpu(), );
  if (!resetShadowDataBufferIfNecessary())
  {
    logerr("DynamicLightShadows: failed to reset the shadow data buffer");
    return;
  }
  if (!shadowSystem)
    return;

  Tab<Point4> slots(framemem_ptr());
  uint32_t firstSlot = 0, nextSlot = 0;
  bool updateSucceeded = true;
  const auto flushSlots = [&]() {
    if (slots.empty())
      return;
    updateSucceeded &= shadowDataBuffer->updateData(firstSlot * SHADOW_SLOT_POINT4_STRIDE * sizeof(Point4), data_size(slots),
      slots.data(), VBLOCK_WRITEONLY);
    slots.clear();
  };
  shadowSystem->forEachUpdatedShadow([&](uint16_t shadow_id) {
    G_ASSERT_RETURN(shadow_id < MAX_DYNAMIC_LIGHTS_SHADOW_SLOTS, );
    if (slots.empty() || shadow_id != nextSlot)
    {
      flushSlots();
      firstSlot = shadow_id;
    }
    nextSlot = shadow_id + 1;
    write_shadow_slot(shadowSystem, shadow_id, &slots[append_items(slots, SHADOW_SLOT_POINT4_STRIDE)]);
  });
  flushSlots();

  if (updateSucceeded)
    shadowSystem->clearUpdatedShadows();
  else
    logerr("DynamicLightShadows: failed to update the shadow data buffer");
}

void DynamicLightShadows::updateVisibleLightShadowBuffersGPU(Sbuffer *visible_lights_data_buf, Sbuffer *visible_lights_counts_buf)
{
  G_ASSERT_RETURN(isGpu(), );
  TIME_D3D_PROFILE(dynamic_lights_shadows_gather);

  syncShadowDataGPU();

  auto &commonShadowsCB = commonLightShadowsBufferCB[size_t(LightBufferSlot::Main)];
  commonShadowsCB.reallocate(COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE, COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE,
    lightsResMgr->getResName(in_frustum_common_light_shadows_buffer_name(LightBufferSlot::Main)),
    BaseReallocatableLightsConstBuffer::Flags::GpuRead);

  Sbuffer *transitBuf = shadowsTransitBuffer.getBuf();
  Sbuffer *ssssBuf = spotLightSsssShadowDescBuffer.getBuf();

  scene_omni_managed_lightsVarId.set_buffer(omniLightsManager->getSceneManagedLightsBuffer());
  scene_spot_managed_lightsVarId.set_buffer(spotLightsManager->getSceneManagedLightsBuffer());
  visible_lights_data_bufVarId.set_buffer(visible_lights_data_buf);
  visible_lights_counts_bufVarId.set_buffer(visible_lights_counts_buf);
  dynamic_lights_shadows_data_bufVarId.set_buffer(shadowDataBuffer.getBuf());
  dynamic_lights_shadows_transit_bufVarId.set_buffer(transitBuf);
  const D3DRESID prevSsssBufId = spot_lights_ssss_shadow_descVarId.get_buffer();
  spot_lights_ssss_shadow_descVarId.set_buffer(spotLightSsssShadowDescBuffer.getBufId());

  gatherCS.dispatchThreads(ssssBuf ? DYNAMIC_LIGHTS_SHADOWS_GATHER_THREADS_COUNT : COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE, 1, 1);

  scene_omni_managed_lightsVarId.set_buffer(nullptr);
  scene_spot_managed_lightsVarId.set_buffer(nullptr);
  visible_lights_data_bufVarId.set_buffer(nullptr);
  visible_lights_counts_bufVarId.set_buffer(nullptr);
  dynamic_lights_shadows_data_bufVarId.set_buffer(nullptr);
  dynamic_lights_shadows_transit_bufVarId.set_buffer(nullptr);
  spot_lights_ssss_shadow_descVarId.set_buffer(prevSsssBufId);

  Sbuffer *cbBuf = commonShadowsCB.getBuf();
  Sbuffer *copyBarrierBufs[] = {transitBuf, cbBuf};
  ResourceBarrier copyBarrierStates[] = {RB_RO_COPY_SOURCE, RB_RW_COPY_DEST};
  d3d::resource_barrier({copyBarrierBufs, copyBarrierStates});
  transitBuf->copyTo(cbBuf, 0, 0, COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE * sizeof(Point4));
  d3d::resource_barrier({cbBuf, RB_RO_CONSTANT_BUFFER | RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});

  if (ssssBuf)
    d3d::resource_barrier({ssssBuf, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
}

template <typename LightsManagerT>
void DynamicLightShadows::fillLightShadowData(LightsManagerT *lights_manager, ShadowSystem *shadow_system,
  CommonLightShadowsDataType &common_shadow_data, const Tab<uint16_t> &visible_light_ids, uint32_t base_index)
{
  constexpr bool isSpot = eastl::is_same_v<LightsManagerT, SpotLightsManager>;
  constexpr uint32_t STRIDE = isSpot ? SPOT_LIGHT_SHADOW_DATA_POINT4_STRIDE : OMNI_LIGHT_SHADOW_DATA_POINT4_STRIDE;
  for (int i = 0, ie = visible_light_ids.size(); i < ie; ++i)
  {
    const uint16_t shadowId = lights_manager->getShadowId(visible_light_ids[i]);
    if constexpr (isSpot)
      write_spot_light_shadow_data(shadow_system, shadowId, &common_shadow_data[base_index + i * STRIDE]);
    else
      common_shadow_data[base_index + i * STRIDE] = get_omni_light_shadow_data(shadow_system, shadowId);
  }
}

void DynamicLightShadows::updateCommonLightShadowsConstBufferCPU(CommonLightShadowsCBType &shadow_command_buffer,
  const char *buffer_name, const Tab<uint16_t> &visible_spot_light_ids, const Tab<uint16_t> &visible_omni_light_ids,
  bool always_write_header) const
{
  const uint32_t commonShadowDataSize = COMMON_LIGHTS_SHADOWS_POINT4_HEADER_SIZE +
                                        visible_spot_light_ids.size() * SPOT_LIGHT_SHADOW_DATA_POINT4_STRIDE +
                                        visible_omni_light_ids.size() * OMNI_LIGHT_SHADOW_DATA_POINT4_STRIDE;

  // FIXME: (workaround) buffer is persistent as it referenced by volume lights when data is not updated in clustered lights
  shadow_command_buffer.reallocate(commonShadowDataSize, COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE, lightsResMgr->getResName(buffer_name),
    BaseReallocatableLightsConstBuffer::Flags::DefaultPersistent);

  const bool hasLights = !visible_spot_light_ids.empty() || !visible_omni_light_ids.empty();
  if (!hasLights && !always_write_header)
  {
    shadow_command_buffer.update(nullptr, 0);
    return;
  }

  // Per spot: 4 float4 tex matrix rows + 1 float4 atlas-UV bounds (rectMin.xy, rectMax.xy).
  CommonLightShadowsDataType commonShadowData;
  commonShadowData.resize(commonShadowDataSize);
  commonShadowData[0] = Point4(visible_spot_light_ids.size(), visible_omni_light_ids.size(),
    SPOT_LIGHT_SHADOW_DATA_POINT4_STRIDE * visible_spot_light_ids.size(), 0);

  fillLightShadowData(spotLightsManager, shadowSystem, commonShadowData, visible_spot_light_ids,
    COMMON_LIGHTS_SHADOWS_POINT4_HEADER_SIZE);
  fillLightShadowData(omniLightsManager, shadowSystem, commonShadowData, visible_omni_light_ids,
    COMMON_LIGHTS_SHADOWS_POINT4_HEADER_SIZE + visible_spot_light_ids.size() * SPOT_LIGHT_SHADOW_DATA_POINT4_STRIDE);

  shadow_command_buffer.update(commonShadowData.data(), data_size(commonShadowData));
}

void DynamicLightShadows::updateOutOfFrustumCommonLightShadowsCB(const Tab<uint16_t> &visible_spot_light_ids,
  const Tab<uint16_t> &visible_omni_light_ids)
{
  const bool hasLights = !visible_spot_light_ids.empty() || !visible_omni_light_ids.empty();
  if (hasLights || !outOfFrustumCommonLightShadowsAreEmpty)
    updateCommonLightShadowsConstBufferCPU(outOfFrustumCommonLightShadowsCB, out_of_frustum_common_light_shadows_buffer_name,
      visible_spot_light_ids, visible_omni_light_ids, false);
  outOfFrustumCommonLightShadowsAreEmpty = !hasLights;
}

void DynamicLightShadows::setEmptyOutOfFrustumCommonLightShadowsCB()
{
  outOfFrustumCommonLightShadowsCB.reallocate(COMMON_LIGHTS_SHADOWS_POINT4_HEADER_SIZE, COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE,
    lightsResMgr->getResName(out_of_frustum_common_light_shadows_buffer_name),
    BaseReallocatableLightsConstBuffer::Flags::DefaultPersistent);
  if (!outOfFrustumCommonLightShadowsAreEmpty)
    outOfFrustumCommonLightShadowsCB.update(nullptr, 0);
  outOfFrustumCommonLightShadowsAreEmpty = true;
}

void DynamicLightShadows::afterResetDevice()
{
  shadowDataBufferInitialized = false;
  if (outOfFrustumCommonLightShadowsAreEmpty)
    outOfFrustumCommonLightShadowsCB.update(nullptr, 0);
}

void DynamicLightShadows::updateVisibleLightShadowBuffersCPU(LightBufferSlot current_buffer_slot,
  const Tab<uint16_t> &visible_spot_light_ids, const Tab<uint16_t> &visible_omni_light_ids)
{
  G_ASSERT(!isGpu());

  auto &commonShadowsCB = commonLightShadowsBufferCB[size_t(current_buffer_slot)];
  updateCommonLightShadowsConstBufferCPU(commonShadowsCB, in_frustum_common_light_shadows_buffer_name(current_buffer_slot),
    visible_spot_light_ids, visible_omni_light_ids, true);

  // Secondary views keep the main view SSSS data.
  const int numSpotShadows = min<int>(visible_spot_light_ids.size(), MAX_CLUSTERED_SPOT_LIGHTS);
  if (current_buffer_slot == LightBufferSlot::Main && spotLightSsssShadowDescBuffer && numSpotShadows > 0 && shadowSystem)
  {
    StaticTab<SpotlightShadowDescriptor, MAX_CLUSTERED_SPOT_LIGHTS> spotLightSsssShadowDesc;
    spotLightSsssShadowDesc.resize(numSpotShadows);
    for (int i = 0; i < visible_spot_light_ids.size(); ++i)
    {
      spotLightSsssShadowDesc[i] =
        get_spot_light_ssss_descriptor(shadowSystem, spotLightsManager->getShadowId(visible_spot_light_ids[i]));
    }

    spotLightSsssShadowDescBuffer.getBuf()->updateData(0, numSpotShadows * sizeof(SpotlightShadowDescriptor),
      static_cast<const void *>(spotLightSsssShadowDesc.data()), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }
}

dynamic_shadow_render::QualityParams DynamicLightShadows::getQualityParams() const
{
  dynamic_shadow_render::QualityParams result;
  result.maxShadowsToUpdateOnFrame = maxShadowsToUpdateOnFrame;
  result.maxShadowViewsToUpdateOnFrame = maxShadowViewsToUpdateOnFrame;
  result.maxShadowDist = maxShadowDist;
  return result;
}

void DynamicLightShadows::setNeedSsss(bool need_ssss)
{
  needSsss = need_ssss;
  recreateSpotLightSsssShadowDescBuffer();
}

void DynamicLightShadows::recreateSpotLightSsssShadowDescBuffer()
{
  spotLightSsssShadowDescBuffer.close();
  if (!needSsss)
    return;
  const char *resName = lightsResMgr->getResName(ssss_shadow_desc_buffer_name);
  if (isGpu())
    spotLightSsssShadowDescBuffer = dag::buffers::create_ua_sr_structured(sizeof(SpotlightShadowDescriptor), MAX_CLUSTERED_SPOT_LIGHTS,
      resName, d3d::buffers::Init::Zero, RESTAG_LIGHTS);
  else
    spotLightSsssShadowDescBuffer = dag::create_sbuffer(sizeof(SpotlightShadowDescriptor), MAX_CLUSTERED_SPOT_LIGHTS,
      SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, resName, RESTAG_LIGHTS);
}
