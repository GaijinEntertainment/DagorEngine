//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <render/lights/omniLight.h>
#include <render/lights/spotLight.h>
#include <render/lights/shadowCastersFlags.h>
#include <render/lights/dynamicShadowRenderExtensions.h>

class ShadowSystem;

class BaseLightsManager
{
protected:
  BaseLightsManager(const char *type_prefix);
  void resizeDynamicShadowIds(uint32_t light_id);
  void invalidateShadowVolume(uint32_t shadow_id);

  ShadowSystem *shadowSystem = nullptr;

public:
  static constexpr int INVALID_SHADOW_VOLUME_ID = 0xFFFF;
  void setShadowSystem(ShadowSystem *shadow_system);
  uint32_t getShadowId(uint32_t light_id) const;
  void closeShadows();
  virtual void updateShadowVolume(uint32_t light_id) = 0;
  virtual vec4f getBoundingSphere(uint32_t light_id) const = 0;
  bool isShadowVolumeAllocated(uint32_t light_id) const;
  bool isShadowClose(uint32_t light_id, const Point3 &view_pos, float max_shadow_dist) const;
  uint32_t allocateShadowVolume(uint32_t light_id, ShadowCastersFlag casters, bool hint_dynamic, uint16_t quality, uint8_t priority,
    uint8_t max_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects);

  void destroyShadowVolume(uint32_t light_id);

private:
  const char *typePrefix;
  Tab<uint16_t> dynamicLightsShadowsIds;
};

template <typename RawLightT>
class LightsManager : public BaseLightsManager
{
protected:
  LightsManager(const char *type_prefix);

public:
  bool tryInvalidateShadowsIfNeed(uint32_t light_id, const RawLightT &new_light);
  virtual const RawLightT &getLight(uint32_t light_id) const = 0;

private:
  static bool isInvalidatingShadowsNeed(const RawLightT &old_light, const RawLightT &new_light);
};

template <typename RawLightT>
LightsManager<RawLightT>::LightsManager(const char *type_prefix) : BaseLightsManager(type_prefix)
{}

template <typename RawLightT>
bool LightsManager<RawLightT>::tryInvalidateShadowsIfNeed(uint32_t light_id, const RawLightT &new_light)
{
  const auto shadowId = getShadowId(light_id);
  if (shadowSystem != nullptr && shadowId != INVALID_SHADOW_VOLUME_ID)
  {
    if (isInvalidatingShadowsNeed(getLight(light_id), new_light))
    {
      invalidateShadowVolume(shadowId);
      return true;
    }
  }

  return false;
};