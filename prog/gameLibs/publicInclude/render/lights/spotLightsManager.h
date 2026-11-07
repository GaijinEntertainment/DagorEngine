//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "spotLight.h"
#include <vecmath/dag_vecMathDecl.h>
#include <EASTL/bit.h>
#include <generic/dag_bitset.h>
#include <generic/dag_tabFwd.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_half.h>
#include <osApiWrappers/dag_spinlock.h>
#include "renderLights.hlsli"
#include <render/lights/lightsManager.h>
#include <render/iesTextureManager.h>

#include "light_mask_inc.hlsli"
#include "spot_light_shadow_flags.hlsli"

SpotLightMaskType &operator|=(SpotLightMaskType &lhs, SpotLightMaskType rhs);

struct Frustum;
class OmniShadowMap;
class Occlusion;
class LightsPartition;

// see the thread-safety NOTE on LightsManager in lightsManager.h
class SpotLightsManager final : public LightsManager<SpotLight, RenderSpotLight, SpotLightMaskType, MAX_SCENE_SPOT_LIGHTS>
{
public:
  explicit SpotLightsManager(LightsManagerSharedStorage *shared_storage);

  void renderDebugBboxes();
  int addLight(const Light &light); // return -1 if fails
  void afterLightAllocation(unsigned int id) override;
  void beforeLightDeallocation(unsigned int id) override;
  void destroyLight(unsigned int id);

  const Light &getLight(unsigned int id) const override;
  void setLight(unsigned int id, const Light &l);
  RenderSpotLight getRenderLight(unsigned int id) const override;

  void updateBoundingSphere(unsigned id);
  void updateBoundingBox(unsigned id);
  bbox3f getBoundingBox(unsigned id) const;
  vec4f getBoundingSphere(unsigned id) const override;

  // light_up_dir is only used if texture id is also provided
  int addLight(const Point3 &pos, const Color3 &color, const Point3 &dir, const float angle, float radius, float attenuation_k = 1.f,
    bool contact_shadows = false, const Point3 &light_up_dir = Point3(0, 1, 0), int tex = -1, float illuminating_plane = 0);

  void setLightPos(unsigned int id, const Point3 &pos);
  Point3 getLightPos(unsigned int id) const;
  Point4 getLightPosRadius(unsigned int id) const;
  void getLightView(unsigned int id, mat44f &viewITM);
  void getLightPersp(unsigned int id, mat44f &proj);
  void setLightDirAngle(unsigned int id, const Point4 &dir_tanHalfAngle, const Point3 &light_up_dir);
  const Point4 &getLightDirAngle(unsigned int id) const;
  void setLightCol(unsigned int id, const Color3 &col);
  void setLightPosAndCol(unsigned int id, const Point3 &pos, const Color3 &color);
  void setLightRadius(unsigned int id, float radius);
  void setLightCullingRadius(unsigned int id, float radius);
  void setLightShadows(unsigned int id, bool shadows);

  // nonOptLightIds packs multiple lightIds per word, so a non-atomic set() for one id
  // can race a concurrent set() for another id sharing that word. resetLightOptimization/
  // setLightOptimized are reached both from setters called under the caller's lightLock
  // and from the shadow readback completion path (DistanceReadbackLights::completeQuery),
  // which runs without lightLock, so nonOptLightIdsLock guards this state on its own instead
  // of relying on the caller.
  bool isLightNonOptimized(int id);
  bool tryGetNonOptimizedLightId(int &id);
  void setLightOptimized(int id);

  void updateShadowVolume(uint32_t light_id) override;

private:
  void resetLightOptimization(int id);

  carray<vec4f, MAX_LIGHTS> boundingSpheres;
  carray<bbox3f, MAX_LIGHTS> boundingBoxes;
  alignas(16) carray<float, MAX_LIGHTS> cosHalfAngles;
  OSSpinlock nonOptLightIdsLock;
  Bitset<MAX_LIGHTS> nonOptLightIds DAG_TS_GUARDED_BY(nonOptLightIdsLock);
};
