//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "omniLight.h"
#include <atomic>
#include <osApiWrappers/dag_spinlock.h>
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_tabFwd.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <3d/dag_texMgr.h>
#include <math/dag_hlsl_floatx.h>
#include <util/dag_oaHashNameMap.h>
#include "renderLights.hlsli"

#include <render/iesTextureManager.h>

#include <EASTL/bitset.h>
#include <EASTL/vector.h>

struct Frustum;
class Occlusion;

/* NOTE: thread safe for OmniLightsManager
  - OmniLightsManager doesn't allocate memory: all data arrays
    (rawLights, lightPriority, etc) are inside instance, not heap.
    It is intended to make some operations thread safe.
  - addLight, destroyLight can be called concurrent
  - destroyLight, setters and getters (setLightMask, getLightMask, etc)
    can be called from several threads, but lightId should be not equal
    for different threads.
  - However, access to the same lightId is not thread safe:
    if some thread updates or destroys some light,
    another thread cannot access to the same lightId.
  - prepare, drawDebugInfo, renderDebugBboxes is not thread safe:
    it should be synchronized with previous writes from another thread.
  - destroyAllLights, close and destructor should be synchronized
    with previous access to OmniLightsManager from another thread.
 */

class OmniLightsManager
{
public:
  typedef OmniLight Light;
  typedef Light RawLight;
  typedef uint8_t mask_type_t;

  static constexpr int MAX_LIGHTS = 2048;
  enum
  {
    GI_LIGHT_MASK = 0x1, // TODO: maybe rename it
    MASK_LENS_FLARE = 0x2,
    MASK_ALL = 0xFF
  };

  OmniLightsManager();
  ~OmniLightsManager();

  void close();

  void prepare(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *, bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane,
    const StaticTab<uint16_t, MAX_LIGHTS> &shadow, float markSmallLightsAsFarLimit = 0, vec3f cameraPos = v_zero(),
    mask_type_t accept_mask = MASK_ALL) const;

  void prepare(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane, Occlusion *,
    bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane, const StaticTab<uint16_t, MAX_LIGHTS> &shadow,
    float markSmallLightsAsFarLimit = 0, vec3f cameraPos = v_zero(), mask_type_t accept_mask = MASK_ALL) const;

  void prepare(const Frustum &frustum, Tab<uint16_t> &lights_with_camera_inside, Tab<uint16_t> &lights_with_camera_outside,
    Occlusion *, bbox3f &inside_box, bbox3f &outside_box, const StaticTab<uint16_t, MAX_LIGHTS> &shadow,
    float markSmallLightsAsFarLimit = 0, vec3f cameraPos = v_zero(), mask_type_t accept_mask = MASK_ALL) const;

  void drawDebugInfo();
  void renderDebugBboxes();

  // priority: lesser = better. if returns -1 - no indices
  int addLight(int priority, const Point3 &pos, const Color3 &color, float radius, float attenuation_k = 1.f)
  {
    return addLight(priority, Light(pos, color, radius, attenuation_k));
  }
  int addLight(int priority, const Point3 &pos, const Color3 &color, float radius, const TMatrix &box, float attenuation_k = 1.f)
  {
    return addLight(priority, Light(pos, color, radius, attenuation_k, box));
  }
  int addLight(int priority, const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex,
    float attenuation_k = 1.f);
  int addLight(int priority, const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex, const TMatrix &box,
    float attenuation_k = 1.f);

  int addLight(int priority, const Light &l);

  void destroyLight(unsigned int id);
  void destroyAllLights();

  void setLightPos(unsigned int id, const Point3 &pos)
  {
    if (check_nan(pos.x + pos.y + pos.z))
    {
      G_ASSERTF(0, "nan in setLightPos");
      return;
    }
    rawLights[id].pos_radius.x = pos.x;
    rawLights[id].pos_radius.y = pos.y;
    rawLights[id].pos_radius.z = pos.z;
  }
  void setLightCol(unsigned int id, const Color3 &col)
  {
    rawLights[id].color_atten.r = col.r;
    rawLights[id].color_atten.g = col.g;
    rawLights[id].color_atten.b = col.b;
  }
  void setLightPosAndCol(unsigned int id, const Point3 &pos, const Color3 &color)
  {
    setLightPos(id, pos);
    setLightCol(id, color);
  }
  void setLightRadius(unsigned int id, float radius)
  {
    if (check_nan(radius))
    {
      G_ASSERTF(0, "nan in setLightRadius");
      return;
    }
    rawLights[id].pos_radius.w = radius;
  }
  void setLightBox(unsigned int id, const TMatrix &box) { rawLights[id].setBox(box); }

  void setLightDirection(unsigned int id, const Point3 &dir) { rawLights[id].setDirection(dir); }

  void setLightTexture(unsigned int id, int tex);

  mask_type_t getLightMask(unsigned int id) const { return masks[id]; }
  void setLightMask(unsigned int id, mask_type_t mask) { masks[id] = mask; }

  const Light &getLight(unsigned int id) const { return rawLights[id]; }
  void setLight(unsigned int id, const Light &l)
  {
    if (check_nan(l.pos_radius.x + l.pos_radius.y + l.pos_radius.z + l.pos_radius.w))
    {
      G_ASSERTF(0, "nan in setLight");
      return;
    }
    rawLights[id] = l;
  }
  int maxIndex() const DAG_TS_NO_THREAD_SAFETY_ANALYSIS { return maxLightIndex; }

  IesTextureCollection::PhotometryData getPhotometryData(int texId) const;

  RenderOmniLight getRenderLight(unsigned int id) const
  {
    const Light &l = rawLights[id];
    RenderOmniLight ret;
    ret.posRadius = l.pos_radius;
    ret.colorFlags = Point4::rgba(l.color_atten);
    ret.direction__tex_scale = l.dir__tex_scale;
    ret.boxR0 = l.boxR0;
    ret.boxR1 = l.boxR1;
    ret.boxR2 = l.boxR2;
    ret.posRelToOrigin_cullRadius = l.posRelToOrigin_cullRadius;
    return ret;
  }

  vec4f getBoundingSphere(unsigned id) const { return ((const vec4f &)rawLights[id].pos_radius); }

private:
  carray<Light, MAX_LIGHTS> rawLights;
  carray<uint8_t, MAX_LIGHTS> lightPriority;
  // masks allows to ignore specific lights in specific cases
  // for example, we can ignore highly dynamic lights for GI
  carray<mask_type_t, MAX_LIGHTS> masks; //-V730_NOINIT
  StaticTab<uint16_t, MAX_LIGHTS> freeLightIds DAG_TS_GUARDED_BY(lightAllocationSpinlock);
  IesTextureCollection *photometryTextures = nullptr;
  OSSpinlock lightAllocationSpinlock;
  int maxLightIndex DAG_TS_GUARDED_BY(lightAllocationSpinlock) = -1;
};
