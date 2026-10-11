// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsManager.h>
#include <render/lights/shadowSystem.h>

BaseLightsManager::BaseLightsManager(const char *name) : name(name) { photometryTextures = IesTextureCollection::acquireRef(); }

DynLightsFeatures BaseLightsManager::getRuntimeFeatures() const
{
  return (sceneManagedLightsBuffer && sceneRenderLightsBuffer) ? DynLightsFeatures::GpuLightsManagerPersistentStorage
                                                               : DynLightsFeatures::CpuOnly;
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
  for (int i = 0; i < dynamicLightsShadowsIds.size(); ++i)
  {
    const auto shadowId = dynamicLightsShadowsIds[i];
    if (shadowId != INVALID_SHADOW_VOLUME_ID)
    {
      if (shadowSystem)
        shadowSystem->destroyVolume(shadowId);
      clearShadowVolumeSlot(i);
    }
  }

  shadowSystem = nullptr;
}

void BaseLightsManager::clearShadowVolumeSlot(uint32_t light_id)
{
  if (light_id < dynamicLightsShadowsIds.size())
    dynamicLightsShadowsIds[light_id] = INVALID_SHADOW_VOLUME_ID;
  OnShadowVolumeDestroyed(light_id);
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
  OnShadowVolumeAllocated(light_id);
  return shadowId;
}

bool BaseLightsManager::isShadowVolumeAllocated(uint32_t light_id) const { return getShadowId(light_id) != INVALID_SHADOW_VOLUME_ID; }

void BaseLightsManager::destroyShadowVolume(uint32_t light_id)
{
  G_ASSERT(shadowSystem);
  G_ASSERTF_RETURN(isShadowVolumeAllocated(light_id), , "%s shadow for light %d not found", name.c_str(), light_id);
  shadowSystem->destroyVolume(getShadowId(light_id));
  clearShadowVolumeSlot(light_id);
}

Sbuffer *BaseLightsManager::getSceneManagedLightsBuffer() { return sceneManagedLightsBuffer.getBuf(); }

Sbuffer *BaseLightsManager::getSceneRenderLightsBuffer() { return sceneRenderLightsBuffer.getBuf(); }
