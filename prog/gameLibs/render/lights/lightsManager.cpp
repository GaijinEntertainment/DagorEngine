// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsManager.h>
#include <render/lights/shadowSystem.h>
#include <render/lights/omniLight.h>
#include <render/lights/spotLight.h>

BaseLightsManager::BaseLightsManager(const char *name) : name(name) { photometryTextures = IesTextureCollection::acquireRef(); }

bool BaseLightsManager::isGPUManagementEnabled() const
{
  return sceneManagedLightsBuffer && sceneManagedLightsCountBuffer && sceneRenderLightsBuffer;
}

BaseLightsManager::~BaseLightsManager()
{
  if (photometryTextures)
  {
    IesTextureCollection::releaseRef();
    photometryTextures = nullptr;
  }
}

IesTextureCollection::PhotometryData BaseLightsManager::getPhotometryData(int texId) const
{
  return photometryTextures->getTextureData(texId);
}

void BaseLightsManager::resizeDynamicShadowIds(uint32_t light_id)
{
  if (dynamicLightsShadowsIds.size() <= light_id)
  {
    int start = append_items(dynamicLightsShadowsIds, light_id - dynamicLightsShadowsIds.size() + 1);
    memset(dynamicLightsShadowsIds.data() + start, 0xFF,
      (dynamicLightsShadowsIds.size() - start) * elem_size(dynamicLightsShadowsIds));
  }
}

void BaseLightsManager::setShadowSystem(ShadowSystem *shadow_system) { shadowSystem = shadow_system; }

void BaseLightsManager::invalidateShadowVolume(uint32_t shadow_id) { shadowSystem->invalidateVolumeShadow(shadow_id); }

void BaseLightsManager::closeShadows()
{
  for (uint16_t &shadowIdx : dynamicLightsShadowsIds)
  {
    if (shadowIdx != INVALID_SHADOW_VOLUME_ID)
    {
      if (shadowSystem)
        shadowSystem->destroyVolume(shadowIdx);

      shadowIdx = INVALID_SHADOW_VOLUME_ID;
    }
  }
  shadowSystem = nullptr;
}

uint32_t BaseLightsManager::getShadowId(uint32_t light_id) const
{
  if (light_id >= dynamicLightsShadowsIds.size())
    return INVALID_SHADOW_VOLUME_ID;

  return dynamicLightsShadowsIds[light_id];
}

uint32_t BaseLightsManager::allocateShadowVolume(uint32_t light_id, ShadowCastersFlag casters, bool hint_dynamic, uint16_t quality,
  uint8_t priority, uint8_t max_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects)
{
  G_ASSERT(shadowSystem);
  G_ASSERTF_RETURN(light_id >= dynamicLightsShadowsIds.size() || dynamicLightsShadowsIds[light_id] == INVALID_SHADOW_VOLUME_ID,
    INVALID_SHADOW_VOLUME_ID, "%s light %d already has shadow", name.c_str(), light_id);
  resizeDynamicShadowIds(light_id);
  const auto shadowId = shadowSystem->allocateVolume(casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
  if (shadowId < 0)
    return INVALID_SHADOW_VOLUME_ID;

  dynamicLightsShadowsIds[light_id] = shadowId;
  return shadowId;
}

bool BaseLightsManager::isShadowVolumeAllocated(uint32_t light_id) const { return getShadowId(light_id) != INVALID_SHADOW_VOLUME_ID; }

void BaseLightsManager::destroyShadowVolume(uint32_t light_id)
{
  G_ASSERT(shadowSystem);
  G_ASSERTF_RETURN(isShadowVolumeAllocated(light_id), , "%s shadow for light %d not found", name.c_str(), light_id);
  shadowSystem->destroyVolume(getShadowId(light_id));
  if (light_id < dynamicLightsShadowsIds.size())
    dynamicLightsShadowsIds[light_id] = INVALID_SHADOW_VOLUME_ID;
}

bool BaseLightsManager::isShadowClose(uint32_t light_id, const Point3 &view_pos, float max_shadow_dist) const
{
  vec4f vposMaxShadow = v_make_vec4f(view_pos.x, view_pos.y, view_pos.z, -max_shadow_dist);
  vec4f mulFactor = v_make_vec4f(1, 1, 1, -1);

  // only if distance to light is closer than max shadow see distance we need update shadow
  vec4f bounding = getBoundingSphere(light_id);
  bounding = v_sub(bounding, vposMaxShadow);
  bounding = v_mul(bounding, bounding);
  bounding = v_dot4_x(bounding, mulFactor);
  return v_test_vec_x_lt_0(bounding);
}

Sbuffer *BaseLightsManager::getSceneManagedLightsBuffer() { return sceneManagedLightsBuffer.getBuf(); }

Sbuffer *BaseLightsManager::getSceneManagedLightsCountBuffer() { return sceneManagedLightsCountBuffer.getBuf(); }

Sbuffer *BaseLightsManager::getSceneRenderLightsBuffer() { return sceneRenderLightsBuffer.getBuf(); }

template <>
bool LightsManager<OmniLight, RenderOmniLight, OmniLightMaskType, MAX_SCENE_OMNI_LIGHTS>::isInvalidatingShadowsNeed(
  const OmniLight &old_light, const OmniLight &new_light)
{
  return !are_approximately_equal(old_light.pos_radius, new_light.pos_radius, eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.shadowNearFarClippingPlanesPad, new_light.shadowNearFarClippingPlanesPad,
           eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.boxR0, new_light.boxR0, eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.boxR1, new_light.boxR1, eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.boxR2, new_light.boxR2, eastl::numeric_limits<float>::epsilon());
}

template <>
bool LightsManager<SpotLight, RenderSpotLight, SpotLightMaskType, MAX_SCENE_SPOT_LIGHTS>::isInvalidatingShadowsNeed(
  const SpotLight &old_light, const SpotLight &new_light)
{
  return !are_approximately_equal(old_light.pos_radius, new_light.pos_radius, eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.shadowNearFarClippingPlanes, new_light.shadowNearFarClippingPlanes,
           eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.texId_scale_illuminatingPlane.z, new_light.texId_scale_illuminatingPlane.z,
           eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.shadowFrustumOffset, new_light.shadowFrustumOffset,
           eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.getShadowTanHalfAngle(), new_light.getShadowTanHalfAngle(),
           eastl::numeric_limits<float>::epsilon()) ||
         !are_approximately_equal(old_light.dir_tanHalfAngle, new_light.dir_tanHalfAngle, eastl::numeric_limits<float>::epsilon());
}