//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "omniLight.h"
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_tabFwd.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <3d/dag_texMgr.h>
#include <math/dag_hlsl_floatx.h>
#include "renderLights.hlsli"
#include <render/lights/lightsManager.h>

#include <EASTL/bitset.h>

#include "light_mask_inc.hlsli"

inline OmniLightMaskType &operator|=(OmniLightMaskType &lhs, OmniLightMaskType rhs)
{
  lhs = static_cast<OmniLightMaskType>(static_cast<eastl::underlying_type<OmniLightMaskType>::type>(lhs) | //-V1016
                                       static_cast<eastl::underlying_type<OmniLightMaskType>::type>(rhs)); //-V1016
  return lhs;
}

struct Frustum;
class Occlusion;
class LightsPartition;

// see the thread-safety NOTE on LightsManager in lightsManager.h
class OmniLightsManager : public LightsManager<OmniLight, RenderOmniLight, OmniLightMaskType, MAX_SCENE_OMNI_LIGHTS>
{
  friend class LightsPartition;

public:
  OmniLightsManager();
  OmniLightsManager(const char *name);

  void drawDebugInfo();
  void renderDebugBboxes();

  // returns -1 if fails
  int addLight(const Point3 &pos, const Color3 &color, float radius, float attenuation_k = 1.f)
  {
    return addLight(Light(pos, color, radius, attenuation_k));
  }
  int addLight(const Point3 &pos, const Color3 &color, float radius, const TMatrix &box, float attenuation_k = 1.f)
  {
    return addLight(Light(pos, color, radius, attenuation_k, box));
  }
  int addLight(const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex, float attenuation_k = 1.f);
  int addLight(const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex, const TMatrix &box,
    float attenuation_k = 1.f);

  int addLight(const Light &l);

  void destroyLight(unsigned int id);

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

  OmniLightMaskType getLightMask(unsigned int id) const { return masks[id]; }
  void setLightMask(unsigned int id, OmniLightMaskType mask) { masks[id] = mask; }

  const Light &getLight(unsigned int id) const override { return rawLights[id]; }
  void setLight(unsigned int id, const Light &l)
  {
    if (check_nan(l.pos_radius.x + l.pos_radius.y + l.pos_radius.z + l.pos_radius.w))
    {
      G_ASSERTF(0, "nan in setLight");
      return;
    }
    rawLights[id] = l;
  }

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
    ret.shadowZnZf_pad = l.shadowNearFarClippingPlanesPad;
    return ret;
  }

  vec4f getBoundingSphere(unsigned id) const override
  {
    const Light &l = rawLights[id];
    const float cullRadius = l.posRelToOrigin_cullRadius.w;
    vec4f bounds = v_ldu(reinterpret_cast<const float *>(&l.pos_radius.x));
    if (cullRadius > 0.f)
      bounds = v_perm_xyzd(bounds, v_splats(cullRadius));
    return bounds;
  }

  void updateShadowVolume(uint32_t light_id) override;
};
