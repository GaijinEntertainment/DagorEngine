//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lights/lightsBase.h>
#include <render/lights/lightsEncoding.h>
#include <render/lights/lightsResources.h>
#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <render/lights/lightsPartition.h>
#include <render/lights/lightsCounter.h>
#include <render/lights/lightsSorter.h>
#include <render/lights/frustumClusters.h>
#include <render/lights/tiledLights.h>
#include <render/lights/clusteredLightsGrid.h>
#include <render/lights/reallocatableLightsConstBuffer.h>
#include <render/lights/lightsRenderer.h>
#include <render/lights/dynamicLightShadows.h>
#include <render/lights/lightsVisibilityChecker.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <math/dag_TMatrix4.h>
#include <generic/dag_tab.h>
#include <EASTL/array.h>
#include <EASTL/optional.h>
#include <math/dag_hlsl_floatx.h>
#include "renderLights.hlsli"
#include <shaders/dag_overrideStates.h>
#include <render/dynamicShadowRender.h>
#include <render/lights/dynamicShadowRenderExtensions.h>
#include <render/lights/shadowCastersFlags.h>
#include <3d/dag_resPtr.h>

class ShaderMaterial;
class ShaderElement;
class ShadowSystem;
class ComputeShaderElement;
class Occlusion;
class DistanceReadbackLights;

struct ClusteredLights
{
  typedef OmniLightsManager::RawLight OmniLight;
  typedef SpotLightsManager::RawLight SpotLight;
  static const int MAX_SHADOW_PRIORITY = 15; // 15 times more important than anything else
  ClusteredLights(const char *name_suffix = "");
  ~ClusteredLights();

  ClusteredLights(const ClusteredLights &) = delete;
  ClusteredLights &operator=(const ClusteredLights &) = delete;
  ClusteredLights(ClusteredLights &&) = delete;
  ClusteredLights &operator=(ClusteredLights &&) = delete;

  // initial_frame_light_count is total visible lights for frame. In 32 (words)
  // shadows_quality is size of dynamic shadow map. 0 means no shadows
  void init(int initial_frame_light_count, uint32_t shadows_quality, bool use_tiled_lights, const char *name_suffix = "");
  void setMaxClusteredDist(const float max_clustered_dist);
  void setMaxShadowDist(const float max_shadow_dist);
  void changeShadowResolution(uint32_t shadows_quality, bool dynamic_shadow_32bit);
  void close();
  void cullFrustumLights(vec3f cur_view_pos, mat44f_cref globtm, mat44f_cref view, mat44f_cref proj, float znear, float zfar,
    Occlusion *occlusion, SpotLightMaskType spot_light_require_any_mask, OmniLightMaskType omni_light_require_any_mask,
    float light_cutoff_dist_sq = 0.f);
  void prepareTiledLights(const bool clear_lights = true);
  void toggleTiledLights(bool use_tiled);
  bool hasDeferredOmniLights() const;
  bool hasDeferredSpotLights() const;
  bool hasClusteredOmniLights() const;
  bool hasClusteredSpotLights() const;
  bool hasDeferredLights() const;
  bool hasClusteredLights() const;

  int getVisibleFarSpotsCount() const;
  int getVisibleFarOmniCount() const;
  int getVisibleClusteredSpotsCount() const;
  int getVisibleClusteredOmniCount() const;
  int getVisibleSpotsCount() const;
  int getVisibleOmniCount() const;

  DynLightsOptimizationMode getLightsCountInterval() const;
  bool cullOutOfFrustumLights(mat44f_cref globtm, SpotLightMaskType spot_light_mask,
    OmniLightMaskType omni_light_mask); // cull without any grid
  void setShadowBias(float z_bias, float slope_z_bias, float shader_z_bias, float shader_slope_z_bias);
  void getShadowBias(float &z_bias, float &slope_z_bias, float &shader_z_bias, float &shader_slope_z_bias) const;
  void setRetainShadowSizeMul(float mul);

  void renderOtherLights();
  void fillAndSetInsideOfFrustumLightsBuffers();
  void setEmptyOutOfFrustumLights();
  void setOutOfFrustumLightsToShader();
  void setInsideOfFrustumLightsToShader() const;

  void selectBufferSlot(LightBufferSlot slot);

  void renderDebugSpotLights();
  void renderDebugOmniLights();
  void renderDebugLights();
  void renderDebugLightsBboxes();

  void destroyLight(uint32_t id);
  uint32_t addOmniLight(const OmniLight &light, OmniLightMaskType mask = OmniLightMaskType::OMNI_LIGHT_MASK_DEFAULT);
  void setLightNoLock(uint32_t id, const OmniLight &light, bool invalidate_shadow) DAG_TS_REQUIRES(lightLock);
  void setLight(uint32_t id, const OmniLight &light, bool invalidate_shadow);
  void setLightWithMask(uint32_t id, const OmniLight &light, OmniLightMaskType mask, bool invalidate_shadow);
  const OmniLight &getOmniLightNoLock(uint32_t id) const DAG_TS_REQUIRES(lightLock);
  OmniLight getOmniLight(uint32_t id) const;
  OmniLightMaskType getOmniLightMask(uint32_t id) const;

  void setLightNoLock(uint32_t id_, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow) DAG_TS_REQUIRES(lightLock);
  void setLight(uint32_t id_, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow);
  uint32_t addSpotLight(const SpotLight &light, SpotLightMaskType mask);
  const SpotLight &getSpotLightNoLock(uint32_t id_) const DAG_TS_REQUIRES(lightLock);
  SpotLight getSpotLight(uint32_t id_) const;
  SpotLightMaskType getSpotLightMask(uint32_t id) const;
  void getSpotLightShadowViewProj(uint32_t id, mat44f &view_itm, mat44f &proj);

  bool isLightVisible(uint32_t id) const;

  // priority - the higher, the better. keep in mind, that with very high value you can steal all updates from other volumes

  // hint_dynamic (not cache static) - light is typically moving, and so will be rendered each frame. Makes sense only
  // only_static_casters == false. only_static_casters - light will not cast shadows from dynamic objects
  // quality - the higher the better. It is the speed of going from lowest mip (min_shadow_size) to high mip
  // (max_shadow_size>>shadow_size_srl).
  //  shadow_size_srl - maximum size degradation (shift right bits count for max shadow. If shadow is 256 maximum, and srl is 2, than
  //  maximum size will be 64)

  bool addShadowToLight(uint32_t id, ShadowCastersFlag casters_flags, bool hint_dynamic, uint16_t quality, uint8_t priority,
    uint8_t shadow_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects);

  bool addShadowToLight(uint32_t id, bool only_static_casters, bool hint_dynamic, uint16_t quality, uint8_t priority,
    uint8_t shadow_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects)
  {
    return addShadowToLight(id, only_static_casters ? ShadowCastersFlag::None : ShadowCastersFlag::Dynamic, hint_dynamic, quality,
      priority, shadow_size_srl, render_gpu_objects);
  }

  void removeShadow(uint32_t id);

  bool getShadowProperties(uint32_t id, ShadowCastersFlag &only_static_casters, bool &hint_dynamic, uint16_t &quality,
    uint8_t &priority, uint8_t &shadow_size_srl, DynamicShadowRenderGPUObjects &render_gpu_objects) const;

  void invalidateAllShadows() DAG_TS_REQUIRES(lightLock); //{ lightShadows->invalidateAllVolumes(); }
  void invalidateStaticObjects(bbox3f_cref box);          // invalidate static content within box
  void shrinkShadowVolumes();                             // release volume capacity retained from peak light count

  using StaticRenderCallback = DynamicLightShadows::StaticRenderCallback;
  using DynamicRenderCallback = DynamicLightShadows::DynamicRenderCallback;

  void framePrepareShadows(dynamic_shadow_render::FrameVolumeData &volume_data, const Point3 &viewPos, mat44f_cref globtm,
    float camera_focal, dag::ConstSpan<bbox3f> dynamicBoxes, bool collect_updates);

  void frameRenderShadows(const dynamic_shadow_render::FrameVolumeData &volume_data, StaticRenderCallback renderStatic,
    DynamicRenderCallback renderDynamic);

  void updateShadowBuffers() DAG_TS_REQUIRES(lightLock);

  dynamic_shadow_render::QualityParams getQualityParams() const;

  void beforeResetDevice();
  void afterResetDevice();
  void setResolution(uint32_t width, uint32_t height);
  void changeResolution(uint32_t width, uint32_t height);

  bbox3f getActiveShadowVolume() const;

  bool initialized() const { return lightsInitialized; }

  void setNeedSsss(bool need_ssss);
  void setMaxShadowsToUpdateOnFrame(int max_shadows);
  void setMaxShadowViewsToUpdateOnFrame(int max_views);

  void resetShadows() DAG_TS_REQUIRES(lightLock);

  mutable OSSpinlock lightLock;

protected:
  LightsResourcesManager lightsResMgr;

  FrustumClusters clusters; //-V730_NOINIT
  static const float MARK_SMALL_LIGHT_AS_FAR_LIMIT;

  void changeShadowResolutionByQuality(uint32_t shadow_quality, bool dynamic_shadow_32bit) DAG_TS_REQUIRES(lightLock);

  Tab<TMatrix4> renderSpotLightsShadows;
  LightBufferSlot currentBufferSlot = LightBufferSlot::Main;

  ReallocatableLightsConstBuffer<sizeof(RenderOmniLight) / 16, true> outOfFrustumOmniLightsCB;
  ReallocatableLightsConstBuffer<sizeof(RenderSpotLight) / 16, true> outOfFrustumVisibleSpotLightsCB;
  Point4 omniOOFBox[2], spotOOFBox[2];
  UniqueBufWithShaderVar outOfFrustumLightsFullGridCB;
  eastl::unique_ptr<ComputeShaderElement> cull_out_of_frustum_lights_cs, clear_out_of_frustum_grid_cs;
  shaders::UniqueOverrideStateId depthBiasOverrideId;
  shaders::UniqueOverrideStateId depthBiasTwoSidedOverrideId;
  shaders::OverrideState depthBiasOverrideState;
  float shaderShadowZBias = 0.001f, shaderShadowSlopeZBias = 0.005f;

  LightsRenderer lightsRenderer;

  OmniLightsManager omniLights DAG_TS_GUARDED_BY(lightLock); //-V730_NOINIT
  SpotLightsManager spotLights DAG_TS_GUARDED_BY(lightLock); //-V730_NOINIT

  DynamicLightShadows dynamicLightShadows DAG_TS_GUARDED_BY(lightLock);
  eastl::optional<LightsVisibilityChecker> lightVisibilityChecker DAG_TS_GUARDED_BY(lightLock);

  LightsPartition lightsPartition;
  LightsCounter lightsCounter;

  float closeSliceDist = 4, maxClusteredDist = 500;
  eastl::unique_ptr<ShadowSystem> lightShadows DAG_TS_PT_GUARDED_BY(lightLock);
  eastl::unique_ptr<DistanceReadbackLights> dstReadbackLights;
  // Per slot: a cull invalidates every slot, each fill consumes only its own.
  eastl::array<bool, LIGHTS_BUFFER_SLOTS> buffersFilled = {};
  bool lightsInitialized = false;
  DynLightsFeatures dynLightsFeatures = DynLightsFeatures::CpuOnly;

  void initClustered(int initial_light_density);
  void initTiledLights();

  eastl::unique_ptr<TiledLights> tiledLights;
  ClusteredLightsGrid lightsGrid;
};
