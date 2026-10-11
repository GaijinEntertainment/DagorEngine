//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_buffers.h>
#include <generic/dag_align.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <render/lights/lightsBase.h>
#include <render/lights/omniLight.h>
#include <render/lights/spotLight.h>
#include <render/lights/shadowCastersFlags.h>
#include <render/lights/dynamicShadowRenderExtensions.h>
#include <osApiWrappers/dag_spinlock.h>
#include <render/iesTextureManager.h>
#include <EASTL/string.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/type_traits.h>
#include <memory/dag_framemem.h>
#include <math/dag_half.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathBase.h>
#include <render/lights/renderLights.hlsli>
#include <render/lights/light_mask_inc.hlsli>
#include <render/lights/lightsManager.hlsli>

inline uint32_t pack_light_source_radius_cm(float radius)
{
  return uint32_t(clamp(int(radius * LIGHT_SOURCE_RADIUS_CM_PER_M + 0.5f), 0, LIGHT_SOURCE_RADIUS_MAX_CM));
}

class ShadowSystem;

class BaseLightsManager : public IDynLightsPipelineSection
{
protected:
  BaseLightsManager(const char *name);
  ~BaseLightsManager();
  void resizeDynamicShadowIds(uint32_t light_id);
  void invalidateShadowVolume(uint32_t shadow_id);

  virtual void OnShadowVolumeAllocated(uint32_t light_id) = 0;
  virtual void OnShadowVolumeDestroyed(uint32_t light_id) = 0;

  eastl::string name;
  ShadowSystem *shadowSystem = nullptr;
  IesTextureCollection *photometryTextures = nullptr;

public:
  static constexpr float FLOAT_EPS = eastl::numeric_limits<float>::epsilon();

  IesTextureCollection::PhotometryData getPhotometryData(int texId) const;
  DynLightsFeatures getRuntimeFeatures() const override;

  void setShadowSystem(ShadowSystem *shadow_system);
  uint32_t getShadowId(uint32_t light_id) const;
  void closeShadows();
  virtual void updateShadowVolume(uint32_t light_id) = 0;
  virtual vec4f getBoundingSphere(uint32_t light_id) const = 0;
  virtual bool isLightValid(uint32_t light_id) const = 0;

  bool isShadowVolumeAllocated(uint32_t light_id) const;
  uint32_t allocateShadowVolume(uint32_t light_id, ShadowCastersFlag casters, bool hint_dynamic, uint16_t quality, uint8_t priority,
    uint8_t max_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects);

  void destroyShadowVolume(uint32_t light_id);

  Sbuffer *getSceneManagedLightsBuffer();
  Sbuffer *getSceneRenderLightsBuffer();

  template <typename Callback>
  void forEachLightsWithCloseShadow(const Point3 &view_pos, float max_shadow_dist, const Callback &callback) const;

protected:
  UniqueBuf sceneManagedLightsBuffer;
  UniqueBuf sceneRenderLightsBuffer;

private:
  void clearShadowVolumeSlot(uint32_t light_id);

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
  virtual RenderLightT getRenderLight(uint32_t light_id) const = 0;

  void markRenderLightDirty(uint32_t light_id);
  void markManagedLightDirty(uint32_t light_id);

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

  void init(DynLightsFeatures requested_features) override;
  bool tryInvalidateShadowsIfNeed(uint32_t light_id, const RawLightT &new_light);
  virtual const RawLightT &getLight(uint32_t light_id) const = 0;
  bool isLightValid(uint32_t light_id) const override;
  int maxIndex() const DAG_TS_NO_THREAD_SAFETY_ANALYSIS;
  void flushUpdatesForGPULights();
  void afterResetDevice();

  ManagedLight getManagedLight(uint32_t light_id) const;
  LightMaskT getLightMask(uint32_t light_id) const;
  void setLightMask(uint32_t light_id, LightMaskT mask);

private:
  static bool isInvalidatingShadowsNeed(const RawLightT &old_light, const RawLightT &new_light);

  template <typename TUpdateData>
  void flushUpdatesForGPULightsImpl(carray<bool, MaxLightsCount> &changed, Sbuffer *buffer);

  void OnShadowVolumeAllocated(uint32_t light_id) final;
  void OnShadowVolumeDestroyed(uint32_t light_id) final;

  OSSpinlock lightAllocationSpinlock;
  StaticTab<uint16_t, MaxLightsCount> freeLightIds DAG_TS_GUARDED_BY(lightAllocationSpinlock);
  int maxLightIndex DAG_TS_GUARDED_BY(lightAllocationSpinlock) = -1;
  bool fullSyncToGPULights = false;

  carray<bool, MaxLightsCount> renderLightChanged;
  carray<bool, MaxLightsCount> managedLightChanged;
};

template <typename Callback>
void BaseLightsManager::forEachLightsWithCloseShadow(const Point3 &view_pos, float max_shadow_dist, const Callback &callback) const
{
  vec4f vposMaxShadow = v_make_vec4f(view_pos.x, view_pos.y, view_pos.z, -max_shadow_dist);
  vec4f mulFactor = v_make_vec4f(1, 1, 1, -1);

  for (uint32_t lightId = 0, ie = dynamicLightsShadowsIds.size(); lightId < ie; ++lightId)
  {
    if (dynamicLightsShadowsIds[lightId] == INVALID_SHADOW_VOLUME_ID)
      continue;

    if (!isLightValid(lightId))
      continue;

    vec4f bounding = getBoundingSphere(lightId);
    bounding = v_sub(bounding, vposMaxShadow);
    bounding = v_mul(bounding, bounding);
    bounding = v_dot4_x(bounding, mulFactor);
    bool isShadowClose = v_test_vec_x_lt_0(bounding);

    if (!isShadowClose)
      continue;

    callback(lightId);
  }
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::LightsManager(const char *name) : BaseLightsManager(name)
{
  G_STATIC_ASSERT(1ULL << (sizeof(*freeLightIds.data()) * 8) >= MAX_LIGHTS);

  mem_set_0(rawLights);
  mem_set_0(masks);
  mem_set_0(renderLightChanged);
  mem_set_0(managedLightChanged);
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
  if (are_approximately_equal(rawLights[id].pos_radius.w, 0.0f, FLOAT_EPS))
    rawLights[id].pos_radius.w = 0.0f;
  masks[id] = mask;
  afterLightAllocation(id);
  markRenderLightDirty(id);
  markManagedLightDirty(id);
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

  markRenderLightDirty(light_id);
  markManagedLightDirty(light_id);

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
bool LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::isLightValid(uint32_t light_id) const
{
  return rawLights[light_id].pos_radius.w > 0.0f;
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
int LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::maxIndex() const DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  return maxLightIndex;
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
LightMaskT LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::getLightMask(uint32_t light_id) const
{
  return masks[light_id];
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::setLightMask(uint32_t light_id, LightMaskT mask)
{
  if (masks[light_id] == mask)
    return;
  masks[light_id] = mask;
  markManagedLightDirty(light_id);
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::init(DynLightsFeatures requested_features)
{
  if (static_cast<bool>(requested_features & DynLightsFeatures::GpuLightsManagerPersistentStorage))
  {
    sceneManagedLightsBuffer = dag::buffers::create_persistent_sr_structured(sizeof(ManagedLight), MaxLightsCount,
      eastl::string(eastl::string::CtorSprintf(), "%s_scene_managed_lights", name.c_str()).c_str(), d3d::buffers::Init::Zero);
    sceneRenderLightsBuffer = dag::buffers::create_persistent_sr_byte_address(
      dag::divide_align_up(uint32_t(sizeof(RenderLightT)) * MaxLightsCount, d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE),
      eastl::string(eastl::string::CtorSprintf(), "%s_scene_render_lights", name.c_str()).c_str(), d3d::buffers::Init::Zero);

    fullSyncToGPULights = false;
  }
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::afterResetDevice()
{
  fullSyncToGPULights = false;
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
ManagedLight LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::getManagedLight(uint32_t light_id) const
{
  return ManagedLight{encode_managed_light(getShadowId(light_id), masks[light_id])};
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::flushUpdatesForGPULights()
{
  G_ASSERT(isGpu());

  if (!fullSyncToGPULights)
  {
    fullSyncToGPULights = true;

    for (uint32_t i = 0; i < MAX_LIGHTS; ++i)
    {
      markManagedLightDirty(i);
      markRenderLightDirty(i);
    }
  }

  flushUpdatesForGPULightsImpl<RenderLightT>(renderLightChanged, sceneRenderLightsBuffer.getBuf());
  flushUpdatesForGPULightsImpl<ManagedLight>(managedLightChanged, sceneManagedLightsBuffer.getBuf());
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::markRenderLightDirty(uint32_t light_id)
{
  if (isGpu())
  {
    renderLightChanged[light_id] = true;
  }
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::markManagedLightDirty(uint32_t light_id)
{
  if (isGpu())
  {
    managedLightChanged[light_id] = true;
  }
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::OnShadowVolumeAllocated(uint32_t light_id)
{
  markManagedLightDirty(light_id);
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::OnShadowVolumeDestroyed(uint32_t light_id)
{
  markManagedLightDirty(light_id);
}

template <typename RawLightT, typename RenderLightT, typename LightMaskT, uint32_t MaxLightsCount>
template <typename TUpdateData>
void LightsManager<RawLightT, RenderLightT, LightMaskT, MaxLightsCount>::flushUpdatesForGPULightsImpl(
  carray<bool, MaxLightsCount> &changed, Sbuffer *buffer)
{
  bool anyChanged = false;
  for (uint32_t id = 0; id < MAX_LIGHTS; ++id)
    if (changed[id])
    {
      anyChanged = true;
      break;
    }
  if (!anyChanged)
    return;

  uint32_t regionStartLightId;
  Tab<TUpdateData> regionToUpdate(framemem_ptr());
  Tab<uint32_t> regionLightIds(framemem_ptr());

  auto flushRegion = [&]() {
    if (regionToUpdate.empty())
      return;
    if (!buffer->updateData(regionStartLightId * sizeof(TUpdateData), data_size(regionToUpdate), regionToUpdate.data(),
          VBLOCK_WRITEONLY))
    {
      logerr("%s: failed to update scene buffer", name.c_str());
      for (uint32_t id : regionLightIds)
        changed[id] = true;
    }
    regionToUpdate.clear();
    regionLightIds.clear();
  };

  for (uint32_t id = 0; id < MAX_LIGHTS; ++id)
  {
    if (!changed[id])
    {
      flushRegion();
      continue;
    }
    changed[id] = false;

    if (regionToUpdate.empty())
      regionStartLightId = id;

    if constexpr (eastl::is_same_v<TUpdateData, ManagedLight>)
      regionToUpdate.push_back(getManagedLight(id));
    else
      regionToUpdate.push_back(getRenderLight(id));
    regionLightIds.push_back(id);
  }

  flushRegion();
}
