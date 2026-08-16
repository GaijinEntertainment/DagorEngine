//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <render/lights/reallocatableLightsConstBuffer.h>
#include <render/lights/lightsResources.h>
#include <render/lights/lightsSorter.h>
#include <drv/3d/dag_buffers.h>
#include <math/dag_frustum.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_resourceTags.h>

class Occlusion;

class LightsPartition
{
public:
  using OmniLightsCB = ReallocatableLightsConstBuffer<sizeof(RenderOmniLight) / 16, true>;
  using SpotLightsCB = ReallocatableLightsConstBuffer<sizeof(RenderSpotLight) / 16, true>;

  // At least on win7 we have a limit for 64k of cb buffer size
  // But drivers requires to keep cb buffer size under 64k on all platforms.
  // So we limit it for all platforms.
  // Reserve one Point4 for the element count constant that ReallocatableLightsConstBuffer
  // prepends when store_elems_count is true (see reallocate()).
  static constexpr int MAX_VISIBLE_FAR_LIGHTS = (65536 - sizeof(Point4)) / max(sizeof(RenderSpotLight), sizeof(RenderOmniLight));


  LightsPartition(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights, const LightsResourcesManager *lights_res_mgr);

  void init(bool use_gpu_partition);
  bool isGPU() const;
  void executeOmniLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    eastl::bitset<OmniLightsManager::MAX_LIGHTS> *visible_id_bitset, Occlusion *, vec4f znear_plane,
    float mark_small_lights_as_far_limit = 0, vec3f camera_pos = v_zero(),
    OmniLightMaskType require_any_mask = OmniLightMaskType::OMNI_LIGHT_MASK_NONE, float cutoff_dist_sq = 0.f) const;
  void executeOmniLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    Occlusion *, vec4f znear_plane, float mark_small_lights_as_far_limit = 0, vec3f camera_pos = v_zero(),
    OmniLightMaskType require_any_mask = OmniLightMaskType::OMNI_LIGHT_MASK_NONE, float cutoff_dist_sq = 0.f) const;
  void executeSpotLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    eastl::bitset<SpotLightsManager::MAX_LIGHTS> *visible_id_bitset, Occlusion *occ, vec4f znear_plane,
    float mark_small_lights_as_far_limit, vec3f camera_pos, SpotLightMaskType require_any_mask, float cutoff_dist_sq = 0.f) const;
  void executeSpotLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    eastl::bitset<SpotLightsManager::MAX_LIGHTS> *visible_id_bitset, Occlusion *occ, vec4f znear_plane,
    SpotLightMaskType require_any_mask, float cutoff_dist_sq = 0.f);
  void prepareClusteredAndFarOmniLightBuffersCPU(const Frustum &frustum, Occlusion *, vec4f znear_plane,
    float mark_small_lights_as_far_limit = 0, vec3f camera_pos = v_zero(),
    OmniLightMaskType require_any_mask = OmniLightMaskType::OMNI_LIGHT_MASK_NONE, float cutoff_dist_sq = 0.f);
  void prepareClusteredAndFarSpotLightBuffersCPU(const Frustum &frustum, Occlusion *occ, vec4f znear_plane,
    float mark_small_lights_as_far_limit, vec3f camera_pos, SpotLightMaskType require_any_mask, float cutoff_dist_sq = 0.f);

  void executeLightsGPUPartition(const Frustum &frustum, vec4f znear_plane, float mark_small_lights_as_far_limit, vec3f camera_pos,
    OmniLightMaskType omni_require_any_mask, SpotLightMaskType spot_require_any_mask, float cutoff_dist_sq = 0.f);

  void close();

  void updateBuffersForVisibleFarLights();
  void updateBuffersForVisibleClusteredLights(int omni_count, int spot_count);

  const Tab<uint16_t> &getVisibleClusteredSpotLightsIds() const;
  const Tab<uint16_t> &getVisibleClusteredOmniLightsIds() const;

  const Tab<RenderOmniLight> &getRenderOmniLightsFar() const;
  const Tab<RenderSpotLight> &getRenderSpotLightsFar() const;

  const Tab<vec4f> &getVisibleClusteredSpotLightsBounds() const;
  const Tab<vec4f> &getVisibleClusteredOmniLightsBounds() const;

  const OmniLightsCB &getVisibleClusteredOmniLightsCB() const;
  const OmniLightsCB &getVisibleFarOmniLightsCB() const;
  const SpotLightsCB &getVisibleClusteredSpotLightsCB() const;
  const SpotLightsCB &getVisibleFarSpotLightsCB() const;

  const UniqueBuf &getVisibleClusteredSpotLightsMasksSB() const;
  const UniqueBuf &getVisibleClusteredOmniLightsMasksSB() const;

  const UniqueBuf &getVisibleClusteredOmniLightsIdsBuffer() const;
  const UniqueBuf &getVisibleClusteredOmniLightsCountBuffer() const;

  const UniqueBuf &getVisibleClusteredSpotLightsIdsBuffer() const;
  const UniqueBuf &getVisibleClusteredSpotLightsCountBuffer() const;

  const UniqueBuf &getVisibleFarOmniLightsIdsBuffer() const;
  const UniqueBuf &getVisibleFarOmniLightsCountBuffer() const;

  const UniqueBuf &getVisibleFarSpotLightsIdsBuffer() const;
  const UniqueBuf &getVisibleFarSpotLightsCountBuffer() const;

  bool isLightVisible(uint32_t id) const;

private:
  template <typename LightsManager>
  static void executeLightsCPUPartition(const LightsManager *lights_manager, const Frustum &frustum, Tab<uint16_t> &lights_inside,
    Tab<uint16_t> &lights_outside, eastl::bitset<LightsManager::MAX_LIGHTS> *visible_id_bitset, Occlusion *occlusion,
    vec4f znear_plane, float mark_small_lights_as_far_limit, vec3f camera_pos, typename LightsManager::MaskType require_any_mask,
    float cutoff_dist_sq);

  void trimVisibleLightsIdLists(Tab<uint16_t> &clustered_lights, Tab<uint16_t> &far_lights, int max_clustered_count);

  template <typename LightsManager>
  static void fillDerivativeLightsLists(const LightsManager *lights_manager, const Tab<uint16_t> &clustered_lights_ids,
    const Tab<uint16_t> &far_lights_ids, Tab<typename LightsManager::RenderLight> &clustered_render_lights,
    Tab<typename LightsManager::RenderLight> &far_render_lights, Tab<typename LightsManager::MaskType> &clustered_lights_masks,
    Tab<vec4f> &clustered_lights_bounds, typename LightsManager::MaskType default_mask_type);

  OmniLightsManager *omniLights;
  SpotLightsManager *spotLights;

  const LightsResourcesManager *lightsResMgr;

  LightsSorter lightsSorter;
  bool useGPUPartition = false;

  Tab<uint16_t> visibleClusteredSpotLightsIds;
  Tab<uint16_t> visibleClusteredOmniLightsIds;

  Tab<uint16_t> visibleFarSpotLightsIds;
  Tab<uint16_t> visibleFarOmniLightsIds;

  eastl::bitset<OmniLightsManager::MAX_LIGHTS> visibleOmniLightsIdSet;
  eastl::bitset<SpotLightsManager::MAX_LIGHTS> visibleSpotLightsIdSet;

  Tab<RenderOmniLight> renderOmniLightsClustered, renderOmniLightsFar;
  Tab<RenderSpotLight> renderSpotLightsClustered, renderSpotLightsFar;

  Tab<SpotLightMaskType> visibleSpotLightsMasks;
  Tab<OmniLightMaskType> visibleOmniLightsMasks;

  Tab<vec4f> visibleClusteredSpotLightsBounds;
  Tab<vec4f> visibleClusteredOmniLightsBounds;

  OmniLightsCB visibleClusteredOmniLightsCB, visibleFarOmniLightsCB;
  SpotLightsCB visibleClusteredSpotLightsCB, visibleFarSpotLightsCB;

  UniqueBuf visibleClusteredSpotLightsMasksSB;
  UniqueBuf visibleClusteredOmniLightsMasksSB;

  ComputeShader partitionOmniCS;
  ComputeShader partitionSpotCS;

  UniqueBuf frustumPlanesCB;

  UniqueBuf visibleClusteredOmniLightsIdsBuffer;
  UniqueBuf visibleClusteredOmniLightsCountBuffer;

  UniqueBuf visibleClusteredSpotLightsIdsBuffer;
  UniqueBuf visibleClusteredSpotLightsCountBuffer;

  UniqueBuf visibleFarOmniLightsIdsBuffer;
  UniqueBuf visibleFarOmniLightsCountBuffer;

  UniqueBuf visibleFarSpotLightsIdsBuffer;
  UniqueBuf visibleFarSpotLightsCountBuffer;
};
