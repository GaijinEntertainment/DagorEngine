//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lights/lightsBase.h>
#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <render/lights/reallocatableLightsConstBuffer.h>
#include <render/lights/lightsResources.h>
#include <render/lights/lightsSorter.h>
#include <render/lights/lightsVisibilityChecker.h>
#include <drv/3d/dag_buffers.h>
#include <math/dag_frustum.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <3d/dag_resourceTags.h>
#include <EASTL/array.h>

class LightsPartition final : public IDynLightsPipelineSection
{
public:
  using OmniLightsCB = ReallocatableLightsConstBuffer<sizeof(RenderOmniLight) / 16, true>;
  using SpotLightsCB = ReallocatableLightsConstBuffer<sizeof(RenderSpotLight) / 16, true>;

  LightsPartition(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights, const LightsResourcesManager *lights_res_mgr);

  void init(DynLightsFeatures requested_features) override;
  DynLightsFeatures getRuntimeFeatures() const override;
  struct ExecuteLightsCPUPartitionParams
  {
    const LightsVisibilityChecker &checker;
    Tab<uint16_t> &omniLightsFar;
    Tab<uint16_t> &omniLightsClustered;
    Tab<uint16_t> &spotLightsFar;
    Tab<uint16_t> &spotLightsClustered;
  };
  void executeLightsCPUPartition(const ExecuteLightsCPUPartitionParams &params) const;

  void prepareClusteredAndFarLightBuffersCPU(const LightsVisibilityChecker &checker);
  void prepareClusteredAndFarLightBuffersGPU(const LightsVisibilityChecker::TestParameters &test_params, float zfar);

  void executeLightsGPUPartition(const LightsVisibilityChecker::TestParameters &test_params, float zfar, bool update_variables = true);

  void close();

  void updateBuffersForVisibleLightsCPU(LightBufferSlot buffer_slot);
  void updateBuffersForVisibleLightsGPU();

  const Tab<uint16_t> &getVisibleClusteredSpotLightsIds() const;
  const Tab<uint16_t> &getVisibleClusteredOmniLightsIds() const;

  const Tab<uint16_t> &getVisibleFarSpotLightsIds() const;
  const Tab<uint16_t> &getVisibleFarOmniLightsIds() const;

  const Tab<vec4f> &getVisibleClusteredSpotLightsBounds() const;
  const Tab<vec4f> &getVisibleClusteredOmniLightsBounds() const;

  const OmniLightsCB &getVisibleClusteredOmniLightsCB(LightBufferSlot buffer_slot) const;
  const OmniLightsCB &getVisibleFarOmniLightsCB(LightBufferSlot buffer_slot) const;
  const SpotLightsCB &getVisibleClusteredSpotLightsCB(LightBufferSlot buffer_slot) const;
  const SpotLightsCB &getVisibleFarSpotLightsCB(LightBufferSlot buffer_slot) const;

  const UniqueBuf &getVisibleClusteredSpotLightsMasksSB() const;
  const UniqueBuf &getVisibleClusteredOmniLightsMasksSB() const;

  const UniqueBuf &getVisibleLightsDataBuffer() const;
  const UniqueBuf &getVisibleLightsCountsBuffer() const;

private:
  static void executeLightsCPUPartition(LightType light_type, int max_index, const LightsVisibilityChecker &checker,
    Tab<uint16_t> &clustered_lights, Tab<uint16_t> &far_lights);

  template <typename LightsManager>
  static void fillDerivativeLightsLists(const LightsManager *lights_manager, const Tab<uint16_t> &clustered_lights_ids,
    const Tab<uint16_t> &far_lights_ids, Tab<typename LightsManager::RenderLight> &clustered_render_lights,
    Tab<typename LightsManager::RenderLight> &far_render_lights, Tab<typename LightsManager::MaskType> &clustered_lights_masks,
    Tab<vec4f> &clustered_lights_bounds, typename LightsManager::MaskType default_mask_type);

  void updatePartitionFrustumUniforms(const Frustum &frustum, vec4f znear_plane, vec3f camera_pos, float zfar);

  void bindPartitionBuffers(const Frustum &frustum, vec4f znear_plane, vec3f camera_pos, float zfar);
  void unbindPartitionBuffers();

  void handleVisibleLightsIdListsOverflowCPU(Tab<uint16_t> &clustered_lights, Tab<uint16_t> &far_lights, int max_clustered_count);

  size_t getBufferSlotIndex(LightBufferSlot buffer_slot) const;

  OmniLightsManager *omniLights;
  SpotLightsManager *spotLights;

  const LightsResourcesManager *lightsResMgr;

  LightsSorter lightsSorter;

  Tab<uint16_t> visibleClusteredSpotLightsIds;
  Tab<uint16_t> visibleClusteredOmniLightsIds;

  Tab<uint16_t> visibleFarSpotLightsIds;
  Tab<uint16_t> visibleFarOmniLightsIds;

  Tab<RenderOmniLight> renderOmniLightsClustered, renderOmniLightsFar;
  Tab<RenderSpotLight> renderSpotLightsClustered, renderSpotLightsFar;

  Tab<SpotLightMaskType> visibleSpotLightsMasks;
  Tab<OmniLightMaskType> visibleOmniLightsMasks;

  Tab<vec4f> visibleClusteredSpotLightsBounds;
  Tab<vec4f> visibleClusteredOmniLightsBounds;

  eastl::array<OmniLightsCB, LIGHTS_BUFFER_SLOTS> visibleClusteredOmniLightsCB, visibleFarOmniLightsCB;
  eastl::array<SpotLightsCB, LIGHTS_BUFFER_SLOTS> visibleClusteredSpotLightsCB, visibleFarSpotLightsCB;

  UniqueBuf visibleClusteredSpotLightsMasksSB;
  UniqueBuf visibleClusteredOmniLightsMasksSB;

  ComputeShader partitionCS;
  ComputeShader packConstantBuffersCS;

  UniqueBuf frustumPlanesCB;

  UniqueBuf visibleLightsDataBuffer;
  UniqueBuf visibleLightsCountsBuffer;
  UniqueBuf visibleLightsTransitBuffer;
};
