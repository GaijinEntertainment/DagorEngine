//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>
#include <EASTL/bitset.h>
#include <generic/dag_functionRef.h>
#include <generic/dag_tab.h>
#include <render/lights/lightsBase.h>
#include <render/lights/reallocatableLightsConstBuffer.h>
#include <render/lights/lightsResources.h>
#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <render/lights/dynamicShadowRenderExtensions.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_staticTab.h>
#include <render/lights/dynamicLightShadows.hlsli>

class ShadowSystem;

// Every method must run under the owner's lightLock. This class has no reference to that
// lock, so the ClusteredLights member holding it is DAG_TS_GUARDED_BY(lightLock); the few
// ClusteredLights methods that call in here from already single-threaded points are marked
// DAG_TS_NO_THREAD_SAFETY_ANALYSIS there instead of taking the lock again.
class DynamicLightShadows final : public IDynLightsPipelineSection
{
public:
  static constexpr int DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME = 4;

  using CommonLightShadowsCBType = ReallocatableLightsConstBuffer<1, false>;

  using StaticRenderCallback = dag::FunctionRef<void(mat44f_cref globTm, mat44f_cref projTm, const TMatrix &itm, int updateIndex,
    int viewIndex, DynamicShadowRenderGPUObjects render_gpu_objects) const>;
  using DynamicRenderCallback =
    dag::FunctionRef<void(const TMatrix &itm, const mat44f &view_tm, const mat44f &proj_tm, int updateIndex, int viewIndex) const>;

  DynamicLightShadows(const LightsResourcesManager *lights_res_mgr, OmniLightsManager *omni_lights_manager,
    SpotLightsManager *spot_lights_manager);
  DynamicLightShadows(const DynamicLightShadows &) = delete;
  DynamicLightShadows(DynamicLightShadows &&) = delete;
  DynamicLightShadows &operator=(const DynamicLightShadows &) = delete;
  DynamicLightShadows &operator=(DynamicLightShadows &&) = delete;

  void init(DynLightsFeatures requested_features) override;
  DynLightsFeatures getRuntimeFeatures() const override;

  void close();
  void setShadowSystem(ShadowSystem *shadow_system);
  void resetShadowVolumeUpdateFlag(uint16_t shadow_id);
  void resetShadowVolumeUpdateFlags();

  void framePrepareShadows(dynamic_shadow_render::FrameVolumeData &volume_data,
    const dynamic_shadow_render::FramePrepareShadowsParams &params);

  void frameRenderShadows(const dynamic_shadow_render::FrameVolumeData &volume_data, StaticRenderCallback renderStatic,
    DynamicRenderCallback renderDynamic, shaders::OverrideStateId depth_bias_override_id,
    shaders::OverrideStateId depth_bias_two_sided_override_id);

  void updateVisibleLightShadowBuffersCPU(LightBufferSlot current_buffer_slot, const Tab<uint16_t> &visible_spot_light_ids,
    const Tab<uint16_t> &visible_omni_light_ids);
  void syncShadowDataGPU();
  void updateVisibleLightShadowBuffersGPU(Sbuffer *visible_lights_data_buf, Sbuffer *visible_lights_counts_buf);

  void updateOutOfFrustumCommonLightShadowsCB(const Tab<uint16_t> &visible_spot_light_ids,
    const Tab<uint16_t> &visible_omni_light_ids);
  void setEmptyOutOfFrustumCommonLightShadowsCB();
  void afterResetDevice();

  const CommonLightShadowsCBType &getInFrustumCommonLightShadowsCB(LightBufferSlot slot) const;
  const CommonLightShadowsCBType &getOutOfFrustumCommonLightShadowsCB() const;

  void setNeedSsss(bool need_ssss);
  Sbuffer *getShadowDataBuffer() const { return shadowDataBuffer.getBuf(); }
  D3DRESID getSpotLightSsssShadowDescBufferId() const { return spotLightSsssShadowDescBuffer.getBufId(); }

  void setMaxShadowDist(float max_shadow_dist) { maxShadowDist = max_shadow_dist; }
  void setMaxShadowsToUpdateOnFrame(int max_shadows) { maxShadowsToUpdateOnFrame = max_shadows; }
  void setMaxShadowViewsToUpdateOnFrame(int max_views) { maxShadowViewsToUpdateOnFrame = max_views; }
  dynamic_shadow_render::QualityParams getQualityParams() const;

private:
  using CommonLightShadowsDataType = StaticTab<Point4, COMMON_LIGHTS_SHADOWS_POINT4_FULL_SIZE>;

  void setLightShadowVolume(BaseLightsManager *lights_manager, int light_id);
  bool resetShadowDataBufferIfNecessary();
  void recreateSpotLightSsssShadowDescBuffer();

  template <typename LightsManagerT>
  static void fillLightShadowData(LightsManagerT *lights_manager, ShadowSystem *shadow_system,
    CommonLightShadowsDataType &common_shadow_data, const Tab<uint16_t> &visible_light_ids, uint32_t base_index);

  void updateCommonLightShadowsConstBufferCPU(CommonLightShadowsCBType &shadow_command_buffer, const char *buffer_name,
    const Tab<uint16_t> &visible_spot_light_ids, const Tab<uint16_t> &visible_omni_light_ids, bool always_write_header) const;

  bool lazyInitCommonLightShadowsCB(CommonLightShadowsCBType &cb, const char *buffer_name) const;

  OmniLightsManager *omniLightsManager;
  SpotLightsManager *spotLightsManager;
  const LightsResourcesManager *lightsResMgr;
  ShadowSystem *shadowSystem = nullptr;

  int maxShadowsToUpdateOnFrame = DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME; // quality param
  int maxShadowViewsToUpdateOnFrame = 0;                                   // quality param, 0 = unlimited
  float maxShadowDist = 120.f;                                             // quality and scene param

  eastl::bitset<SpotLightsManager::MAX_LIGHTS + OmniLightsManager::MAX_LIGHTS> dynamicLightsShadowsVolumeSet;
  mutable eastl::array<CommonLightShadowsCBType, LIGHTS_BUFFER_SLOTS> commonLightShadowsBufferCB;
  mutable CommonLightShadowsCBType outOfFrustumCommonLightShadowsCB;

  mutable bool outOfFrustumCommonLightShadowsAreEmpty = false;
  UniqueBuf spotLightSsssShadowDescBuffer;
  bool needSsss = false;
  UniqueBuf shadowDataBuffer;
  UniqueBuf shadowsTransitBuffer;
  ComputeShader gatherCS;
  bool shadowDataBufferInitialized = false;
};
