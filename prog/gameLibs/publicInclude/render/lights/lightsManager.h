//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_buffers.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <render/lights/omniLight.h>
#include <render/lights/spotLight.h>
#include <render/lights/shadowCastersFlags.h>
#include <render/lights/dynamicShadowRenderExtensions.h>
#include <osApiWrappers/dag_spinlock.h>
#include <render/iesTextureManager.h>
#include <EASTL/string.h>
#include <math/dag_half.h>
#include <math/dag_hlsl_floatx.h>
#include <render/lights/renderLights.hlsli>
#include <render/lights/light_mask_inc.hlsli>
#include <render/lights/lightsManager.hlsli>

class ShadowSystem;

class BaseLightsManager
{
protected:
  BaseLightsManager(const char *name);
  ~BaseLightsManager();
  void resizeDynamicShadowIds(uint32_t light_id);
  void invalidateShadowVolume(uint32_t shadow_id);

  eastl::string name;

  ShadowSystem *shadowSystem = nullptr;
  IesTextureCollection *photometryTextures = nullptr;

public:
  IesTextureCollection::PhotometryData getPhotometryData(int texId) const;

  bool isGPUManagementEnabled() const;
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

  Sbuffer *getSceneManagedLightsBuffer();
  Sbuffer *getSceneManagedLightsCountBuffer();
  Sbuffer *getSceneRenderLightsBuffer();

protected:
  UniqueBuf sceneManagedLightsBuffer;
  UniqueBuf sceneManagedLightsCountBuffer;
  UniqueBuf sceneRenderLightsBuffer;

private:
  Tab<uint16_t> dynamicLightsShadowsIds;
};

/* NOTE: thread safe for LightsManager derived classes (OmniLightsManager, SpotLightsManager)
  - a derived class doesn't allocate memory: all data arrays
    (rawLights, masks, etc) are inside instance, not heap.
    It is intended to make some operations thread safe.
  - addLight, destroyLight can be called concurrent
  - destroyLight, setters and getters (setLightMask, getLightMask, etc)
    can be called from several threads, but lightId should be not equal
    for different threads.
  - However, access to the same lightId is not thread safe:
    if some thread updates or destroys some light,
    another thread cannot access to the same lightId.
  - the "different lightId" guarantee above assumes the per-light state is
    one array element per lightId (carray). A derived class that packs
    several lightIds into one word (e.g. a bitset) must synchronize that
    state itself; this guarantee does not extend to it.
  - debug draw methods (drawDebugInfo, renderDebugBboxes) are not thread
    safe: they should be synchronized with previous writes from another
    thread.
  - the destructor should be synchronized with previous access
    from another thread.
 */
template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
class LightsManager : public BaseLightsManager
{
protected:
  LightsManager(const char *name);

  int allocateLight(const RawLightT &l, LightMaskT mask);
  void deallocateLight(uint32_t light_id);

  virtual void afterLightAllocation(uint32_t light_id);
  virtual void beforeLightDeallocation(uint32_t light_id);

  carray<RawLightT, MaxLightsCount> rawLights;
  // masks allows to ignore specific lights in specific cases
  // for example, we can ignore highly dynamic lights for GI
  carray<LightMaskT, MaxLightsCount> masks;

public:
  static constexpr int MAX_LIGHTS = MaxLightsCount;

  using MaskType = LightMaskT;
  using RenderLight = RenderLightT;

  using Light = RawLightT;
  using RawLight = Light;

  bool tryInvalidateShadowsIfNeed(uint32_t light_id, const RawLightT &new_light);
  virtual const RawLightT &getLight(uint32_t light_id) const = 0;
  int maxIndex() const DAG_TS_NO_THREAD_SAFETY_ANALYSIS;
  void setUpGpuManagement();

private:
  static bool isInvalidatingShadowsNeed(const RawLightT &old_light, const RawLightT &new_light);
  OSSpinlock lightAllocationSpinlock;
  StaticTab<uint16_t, MaxLightsCount> freeLightIds DAG_TS_GUARDED_BY(lightAllocationSpinlock);
  int maxLightIndex DAG_TS_GUARDED_BY(lightAllocationSpinlock) = -1;
};

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::LightsManager(const char *name) : BaseLightsManager(name)
{
  G_STATIC_ASSERT(1ULL << (sizeof(*freeLightIds.data()) * 8) >= MAX_LIGHTS);

  mem_set_0(rawLights);
  mem_set_0(masks);
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
bool LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::tryInvalidateShadowsIfNeed(uint32_t light_id,
  const RawLightT &new_light)
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

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
int LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::allocateLight(const RawLightT &l, LightMaskT mask)
{
  OSSpinlockScopedLock lock(lightAllocationSpinlock);
  int id = -1;
  if (freeLightIds.size())
  {
    id = freeLightIds.back();
    freeLightIds.pop_back();
  }
  else
  {
    if (maxLightIndex < (MAX_LIGHTS - 1))
      id = ++maxLightIndex;
    else
      logerr("%s light allocation failed, already have %d lights in scene!", name.c_str(), MAX_LIGHTS);
  }
  if (id < 0)
    return id;
  rawLights[id] = l;
  masks[id] = mask;
  afterLightAllocation(id);
  return id;
};

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::deallocateLight(uint32_t light_id)
{
  OSSpinlockScopedLock lock(lightAllocationSpinlock);
  G_ASSERT_RETURN(light_id <= maxLightIndex, );

  beforeLightDeallocation(light_id);

  memset(&rawLights[light_id], 0, sizeof(rawLights[light_id]));
  masks[light_id] = static_cast<LightMaskT>(0);

  if (light_id == maxLightIndex)
  {
    --maxLightIndex;
    return;
  }

#if DAGOR_DBGLEVEL > 0
  for (int i = 0; i < freeLightIds.size(); ++i)
    if (freeLightIds[i] == light_id)
    {
      G_ASSERTF(freeLightIds[i] != light_id, "%s light %d is already destroyed, re-destroy is invalid", name.c_str(), light_id);
      return;
    }
#endif
  freeLightIds.push_back(light_id);
};

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::afterLightAllocation(uint32_t)
{}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::beforeLightDeallocation(uint32_t)
{}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
int LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::maxIndex() const DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  return maxLightIndex;
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::setUpGpuManagement()
{
  sceneManagedLightsBuffer = dag::buffers::create_ua_sr_structured(sizeof(ManagedLight), MaxLightsCount,
    eastl::string(eastl::string::CtorSprintf(), "%s_scene_managed_lights", name.c_str()).c_str());
  sceneRenderLightsBuffer = dag::buffers::create_ua_sr_structured(sizeof(RenderLightT), MaxLightsCount,
    eastl::string(eastl::string::CtorSprintf(), "%s_scene_render_lights", name.c_str()).c_str());
  sceneManagedLightsCountBuffer = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 1,
    eastl::string(eastl::string::CtorSprintf(), "%s_scene_managed_lights_count", name.c_str()).c_str());
}
