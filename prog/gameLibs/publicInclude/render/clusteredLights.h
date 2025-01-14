//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/omniLightsManager.h>
#include <render/spotLightsManager.h>
#include <render/frustumClusters.h>
#include <render/tiledLights.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <math/dag_TMatrix4.h>
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <EASTL/array.h>
#include <EASTL/fixed_function.h>
#include <EASTL/bitset.h>
#include <math/dag_hlsl_floatx.h>
#include "renderLights.hlsli"
#include <shaders/dag_overrideStates.h>
#include <render/dynamicShadowRender.h>
#include <render/dynamicShadowRenderExtensions.h>
#include <render/shadowCastersFlags.h>
#include <3d/dag_resPtr.h>

class ShaderMaterial;
class ShaderElement;
class ShadowSystem;
class ComputeShaderElement;
class Occlusion;
class DistanceReadbackLights;

// Uses optimized shader variants for frames where few lights are
// visible. See dynamic_lights_count shader variable.
// "Few" means less than the threshold, which is the size of a GPU
// word in bits.
// should match render\shaders\dynamic_lights_count.dshl
enum class DynLightsOptimizationMode : int
{
  NO_LIGHTS,
  NO_SPOTS_FEW_OMNI,
  FEW_SPOTS_NO_OMNI,
  FEW_SPOTS_FEW_OMNI,
  FULL_CLUSTERED
};
constexpr static int LIGHTS_OPTIMIZATION_THRESHOLD = 32;

inline DynLightsOptimizationMode get_lights_count_interval(uint32_t spotsCount, uint32_t omniCount)
{
  if (spotsCount == 0 && omniCount == 0)
    return DynLightsOptimizationMode::NO_LIGHTS;

  constexpr int THRESHOLD = LIGHTS_OPTIMIZATION_THRESHOLD;
  if (spotsCount == 0 && omniCount <= THRESHOLD)
    return DynLightsOptimizationMode::NO_SPOTS_FEW_OMNI;
  else if (spotsCount <= THRESHOLD && omniCount == 0)
    return DynLightsOptimizationMode::FEW_SPOTS_NO_OMNI;
  else if (spotsCount <= THRESHOLD && omniCount <= THRESHOLD)
    return DynLightsOptimizationMode::FEW_SPOTS_FEW_OMNI;
  return DynLightsOptimizationMode::FULL_CLUSTERED;
}
struct ClusteredLights
{
  static constexpr int DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME = 4;

  typedef OmniLightsManager::RawLight OmniLight;
  typedef SpotLightsManager::RawLight SpotLight;
  static const int MAX_SHADOW_PRIORITY = 15; // 15 times more imprtant than anything else
  ClusteredLights(const char *name_suffix = "");
  ~ClusteredLights();

  // initial_frame_light_count is total visible lights for frame. In 32 (words)
  // shadows_quality is size of dynmic shadow map. 0 means no shadows
  void init(int initial_frame_light_count, uint32_t shadows_quality, bool use_tiled_lights, const char *name_suffix = "");
  void setMaxClusteredDist(const float max_clustered_dist);
  void setMaxShadowDist(const float max_shadow_dist) { maxShadowDist = max_shadow_dist; }
  void changeShadowResolution(uint32_t shadows_quality, bool dynamic_shadow_32bit);
  void close();
  void cullFrustumLights(vec3f cur_view_pos, mat44f_cref globtm, mat44f_cref view, mat44f_cref proj, float znear, Occlusion *occlusion,
    SpotLightsManager::mask_type_t spot_light_mask, OmniLightsManager::mask_type_t omni_light_mask);
  void prepareTiledLights()
  {
    if (tiledLights)
      tiledLights->computeTiledLigths();
  }
  void toggleTiledLights(bool use_tiled);
  bool hasDeferredLights() const { return renderFarOmniLights.size() + renderFarSpotLights.size() != 0; }
  bool hasClusteredLights() const { return clustersOmniGrid.size() + clustersSpotGrid.size() != 0; }
  int getVisibleNotClusteredSpotsCount() const { return renderFarSpotLights.size(); }
  int getVisibleNotClusteredOmniCount() const { return renderFarOmniLights.size(); }
  int getVisibleClusteredSpotsCount() const { return renderSpotLights.size(); } // fixme
  int getVisibleClusteredOmniCount() const { return renderOmniLights.size(); }
  int getVisibleSpotsCount() const { return getVisibleClusteredSpotsCount() + getVisibleNotClusteredSpotsCount(); }
  int getVisibleOmniCount() const { return getVisibleClusteredOmniCount() + getVisibleNotClusteredOmniCount(); }
  bool cullOutOfFrustumLights(mat44f_cref globtm, SpotLightsManager::mask_type_t spot_light_mask,
    OmniLightsManager::mask_type_t omni_light_mask); // cull without any grid
  void setShadowBias(float z_bias, float slope_z_bias, float shader_z_bias, float shader_slope_z_bias);
  void getShadowBias(float &z_bias, float &slope_z_bias, float &shader_z_bias, float &shader_slope_z_bias) const;

  void renderOtherLights();
  void setBuffersToShader();
  void setEmptyOutOfFrustumLights();
  void setOutOfFrustumLightsToShader();
  void setInsideOfFrustumLightsToShader();

  void renderDebugSpotLights();
  void renderDebugOmniLights();
  void renderDebugLights();
  void renderDebugLightsBboxes();
  void drawDebugClusters(int slice);

  void destroyLight(uint32_t id);
  uint32_t addOmniLight(const OmniLight &light, OmniLightsManager::mask_type_t mask = OmniLightsManager::GI_LIGHT_MASK);
  void setLight(uint32_t id, const OmniLight &light, bool invalidate_shadow);
  void setLightWithMask(uint32_t id, const OmniLight &light, OmniLightsManager::mask_type_t mask, bool invalidate_shadow);
  OmniLight getOmniLight(uint32_t id) const;

  void setLight(uint32_t id_, const SpotLight &light, SpotLightsManager::mask_type_t mask, bool invalidate_shadow);
  uint32_t addSpotLight(const SpotLight &light, SpotLightsManager::mask_type_t mask);
  SpotLight getSpotLight(uint32_t id_) const;

  bool isLightVisible(uint32_t id) const;

  // priority - the higher, the better. keep in mind, that with very high value you can steal all updates from other volumes

  // hint_dyamic (not cache static) - light is typically moving, and so will be rendered each frame. Makes sense only
  // only_static_casters == false. only_static_casters - light will not cast shadows from dynamic objects
  // quality - the higher the better. It is the speed of going from lowest mip (min_shadow_size) to high mip
  // (max_shadow_size>>shadow_size_srl).
  //  shadow_size_srl - maximum size degradation (shft right bits count for max shadow. If shadow is 256 maximum, and srl is 2, than
  //  maximum size will be 64)

  bool addShadowToLight(uint32_t id, ShadowCastersFlag casters_flags, bool hint_dynamic, uint16_t quality, uint8_t priority,
    uint8_t shadow_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects);

  bool addShadowToLight(uint32_t id, bool only_static_casters, bool hint_dynamic, uint16_t quality, uint8_t priority,
    uint8_t shadow_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects)
  {
    return addShadowToLight(id, only_static_casters ? DYNAMIC_CASTERS : STATIC_CASTERS, hint_dynamic, quality, priority,
      shadow_size_srl, render_gpu_objects);
  }

  void removeShadow(uint32_t id);

  bool getShadowProperties(uint32_t id, ShadowCastersFlag &only_static_casters, bool &hint_dynamic, uint16_t &quality,
    uint8_t &priority, uint8_t &shadow_size_srl, DynamicShadowRenderGPUObjects &render_gpu_objects) const;
  enum
  {
    SPOT_LIGHT_FLAG = (1 << 30),
    INVALID_LIGHT = 0xFFFFFFFF & (~SPOT_LIGHT_FLAG)
  };
  void invalidateAllShadows();                   //{ lightShadows->invalidateAllVolumes(); }
  void invalidateStaticObjects(bbox3f_cref box); // invalidate static content within box

  using StaticRenderCallback = void(mat44f_cref globTm, const TMatrix &itm, int updateIndex, int viewIndex,
    DynamicShadowRenderGPUObjects render_gpu_objects);
  using DynamicRenderCallback = void(const TMatrix &itm, const mat44f &view_tm, const mat44f &proj_tm);

  void framePrepareShadows(dynamic_shadow_render::VolumesVector &volumesToRender, const Point3 &viewPos, mat44f_cref globtm, float hk,
    dag::ConstSpan<bbox3f> dynamicBoxes, dynamic_shadow_render::FrameUpdates *frameUpdates);

  void frameRenderShadows(const dag::ConstSpan<uint16_t> &volumesToRender,
    eastl::fixed_function<sizeof(void *) * 2, StaticRenderCallback> renderStatic,
    eastl::fixed_function<sizeof(void *) * 2, DynamicRenderCallback> renderDynamic);

  void updateShadowBuffers();

  dynamic_shadow_render::QualityParams getQualityParams() const;

  void afterReset();
  void setResolution(uint32_t width, uint32_t height);
  void changeResolution(uint32_t width, uint32_t height);

  bbox3f getActiveShadowVolume() const;

  bool initialized() const { return lightsInitialized; }

  void setNeedSsss(bool need_ssss);
  void setMaxShadowsToUpdateOnFrame(int max_shadows) { maxShadowsToUpdateOnFrame = max_shadows; }

  void resetShadows();

protected:
  Tab<uint16_t> visibleSpotLightsId;
  Tab<uint16_t> visibleOmniLightsId;
  eastl::bitset<OmniLightsManager::MAX_LIGHTS> visibleOmniLightsIdSet;
  eastl::bitset<SpotLightsManager::MAX_LIGHTS> visibleSpotLightsIdSet;

  FrustumClusters clusters; //-V730_NOINIT
  static constexpr int MAX_FRAMES = 2;
  static const float MARK_SMALL_LIGHT_AS_FAR_LIMIT;

  // At least on win7 we have a limit for 64k of cb buffer size
  // But drivers requires to keep cb buffer size under 64k on all platforms.
  // So we limit it for all platforms.
  static constexpr int MAX_VISIBLE_FAR_LIGHTS = 65536 / max(sizeof(RenderSpotLight), sizeof(RenderOmniLight));

  static bool reallocate_common(UniqueBuf &buf, uint16_t &size, int target_size_in_constants, const char *stat_name,
    bool permanent = false);
  static bool updateConsts(Sbuffer *buf, void *data, int data_size, int elems_count);
  void changeShadowResolutionByQuality(uint32_t shadow_quality, bool dynamic_shadow_32bit);

  template <int elem_size_in_constants, bool store_elems_count>
  struct ReallocatableConstantBuffer
  {
    bool update(void *data, int data_size)
    {
      G_ASSERT(data_size % ELEM_SIZE_IN_BYTES == 0);
      int elems_count = data_size / ELEM_SIZE_IN_BYTES;
      wasWritten = true;
      return updateConsts(buf.getBuf(), data, data_size, store_elems_count ? elems_count : -1);
    }
    void close()
    {
      buf.close();
      size = 0;
      wasWritten = false;
    }
    ~ReallocatableConstantBuffer() { close(); }
    bool reallocate(int target_size_in_elems, int max_size_in_elems, const char *stat_name, bool persistent = false)
    {
      wasWritten = false;
      int targetSizeInElems = min(target_size_in_elems, max_size_in_elems);
      if (d3d::get_driver_code().is(d3d::metal)) // this is because metal validator. buffer size should match shader code
        targetSizeInElems = max_size_in_elems;
      int targetSizeInConstants = targetSizeInElems * ELEM_SIZE + (store_elems_count ? 1 : 0);
      if (!targetSizeInConstants || size >= targetSizeInConstants)
        return true;
      return ClusteredLights::reallocate_common(buf, size, targetSizeInConstants, stat_name, persistent);
    }
    D3DRESID getId() const
    {
      G_ASSERT(wasWritten || !store_elems_count);
      return buf.getBufId();
    }

  protected:
    enum
    {
      ELEM_SIZE = elem_size_in_constants,
      ELEM_SIZE_IN_BYTES = ELEM_SIZE * sizeof(Point4)
    };
    UniqueBuf buf;
    uint16_t size = 0; // in constants, i.e. 16 bytes*size is size in bytes
    bool wasWritten = false;
  };

  enum class LightType
  {
    Spot,
    Omni,
    Invalid
  };
  struct DecodedLightId
  {
    LightType type;
    uint32_t id;
  };
  __forceinline static DecodedLightId decode_light_id(uint32_t id)
  {
    if (id == INVALID_LIGHT)
      return {LightType::Invalid, uint32_t(INVALID_LIGHT)};

    if (id & SPOT_LIGHT_FLAG)
    {
      id &= ~SPOT_LIGHT_FLAG;
      return {LightType::Spot, id};
    }
    else
      return {LightType::Omni, id};
  }
  __forceinline static uint32_t encode_light_id(LightType type, uint32_t id)
  {
    if (type == LightType::Spot)
      return id | SPOT_LIGHT_FLAG;
    return id;
  }

  Tab<RenderOmniLight> renderOmniLights, renderFarOmniLights;
  Tab<RenderSpotLight> renderSpotLights, renderFarSpotLights;
  Tab<TMatrix4> renderSpotLightsShadows;
  Tab<uint32_t> clustersOmniGrid, clustersSpotGrid;
  Tab<SpotLightsManager::mask_type_t> visibleSpotLightsMasks;
  Tab<OmniLightsManager::mask_type_t> visibleOmniLightsMasks;
  ReallocatableConstantBuffer<sizeof(RenderOmniLight) / 16, true> visibleOmniLightsCB, visibleFarOmniLightsCB;
  ReallocatableConstantBuffer<sizeof(RenderSpotLight) / 16, true> visibleSpotLightsCB, visibleFarSpotLightsCB;
  ReallocatableConstantBuffer<1, false> commonLightShadowsBufferCB;
  UniqueBufHolder spotLightSsssShadowDescBuffer;
  UniqueBuf visibleSpotLightsMasksSB;
  UniqueBuf visibleOmniLightsMasksSB;

  ReallocatableConstantBuffer<sizeof(RenderOmniLight) / 16, true> outOfFrustumOmniLightsCB;
  ReallocatableConstantBuffer<sizeof(RenderSpotLight) / 16, true> outOfFrustumVisibleSpotLightsCB;
  ReallocatableConstantBuffer<1, false> outOfFrustumCommonLightsShadowsCB;
  Point4 omniOOFBox[2], spotOOFBox[2];
  UniqueBufHolder outOfFrustumLightsFullGridCB;
  eastl::unique_ptr<ComputeShaderElement> cull_out_of_frustum_lights_cs, clear_out_of_frustum_grid_cs;

  eastl::array<UniqueBuf, MAX_FRAMES> lightsFullGridCB;
  carray<uint32_t, MAX_FRAMES> currentIndicesSize;
  enum
  {
    NO_CLUSTERED_LIGHTS,
    HAS_CLUSTERED_LIGHTS,
    NOT_INITED
  } gridFrameHasLights = NOT_INITED;
  shaders::UniqueOverrideStateId depthBiasOverrideId;
  shaders::OverrideState depthBiasOverrideState;
  float shaderShadowZBias = 0.001f, shaderShadowSlopeZBias = 0.005f;

  void validateDensity(uint32_t words);
  uint32_t lightsGridFrame = 0, allocatedWords = 0;

  ShaderMaterial *pointLightsMat = 0, *pointLightsDebugMat = 0;
  ShaderElement *pointLightsElem = 0, *pointLightsDebugElem = 0;
  ShaderMaterial *spotLightsMat = 0, *spotLightsDebugMat = 0;
  ShaderElement *spotLightsElem = 0, *spotLightsDebugElem = 0;
  uint32_t v_count = 0, f_count = 0;
  UniqueBuf coneSphereVb;
  UniqueBuf coneSphereIb;
  static constexpr int INVALID_VOLUME = 0xFFFF;

  OmniLightsManager omniLights;                                            //-V730_NOINIT
  SpotLightsManager spotLights;                                            //-V730_NOINIT
  float closeSliceDist = 4, maxClusteredDist = 500;                        //?
  int maxShadowsToUpdateOnFrame = DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME; // quality param
  float maxShadowDist = 120.f;                                             // quality and scene param
  eastl::unique_ptr<ShadowSystem> lightShadows;
  eastl::unique_ptr<DistanceReadbackLights> dstReadbackLights;
  StaticTab<uint16_t, SpotLightsManager::MAX_LIGHTS> dynamicSpotLightsShadows; //-V730_NOINIT
  StaticTab<uint16_t, OmniLightsManager::MAX_LIGHTS> dynamicOmniLightsShadows; //-V730_NOINIT
  eastl::bitset<SpotLightsManager::MAX_LIGHTS + OmniLightsManager::MAX_LIGHTS> dynamicLightsShadowsVolumeSet;
  bool buffersFilled = false;
  bool lightsInitialized = false;
  void setSpotLightShadowVolume(int spot_light_id);
  void setOmniLightShadowVolume(int omni_light_id);

  void initClustered(int initial_light_density);
  void clusteredCullLights(mat44f_cref view, mat44f_cref proj, float znear, float minDist, float maxDist,
    dag::ConstSpan<RenderOmniLight> omni_lights, dag::ConstSpan<SpotLightsManager::RawLight> spot_lights,
    dag::ConstSpan<vec4f> spot_light_bounds, bool use_occlusion, bool &has_omni, bool &has_spot, uint32_t *omni, uint32_t omni_words,
    uint32_t *spot, uint32_t spot_words);
  bool fillClusteredCB(uint32_t *omni, uint32_t omni_words, uint32_t *spot, uint32_t spot_words);
  void closeOmni();
  void closeSpot();
  void closeOmniShadows();
  void closeSpotShadows();
  void initConeSphere();
  void initOmni();
  void initSpot();
  void initDebugOmni();
  void closeDebugOmni();
  void closeDebugSpot();
  void initDebugSpot();
  void fillBuffers();

  void setBuffers();
  void resetBuffers();
  void renderPrims(ShaderElement *elem, int buffer_var_id, D3DRESID replaced_buffer, int inst_count, int vstart, int vcount,
    int index_start, int fcount);

  eastl::unique_ptr<TiledLights> tiledLights;

  eastl::string nameSuffix;
  mutable eastl::string resNameTmp;
  // append nameSuffix to this name to get unique d3d names
  const char *getResName(const char *name) const
  {
    if (DAGOR_LIKELY(nameSuffix.empty()))
      return name;
    resNameTmp = name;
    resNameTmp += nameSuffix;
    return resNameTmp.c_str();
  }
};
