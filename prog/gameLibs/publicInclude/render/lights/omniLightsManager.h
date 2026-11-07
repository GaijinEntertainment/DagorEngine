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

OmniLightMaskType &operator|=(OmniLightMaskType &lhs, OmniLightMaskType rhs);

struct Frustum;
class Occlusion;
class LightsPartition;

// see the thread-safety NOTE on LightsManager in lightsManager.h
class OmniLightsManager final : public LightsManager<OmniLight, RenderOmniLight, OmniLightMaskType, MAX_SCENE_OMNI_LIGHTS>
{
public:
  explicit OmniLightsManager(LightsManagerSharedStorage *shared_storage);

  void drawDebugInfo();
  void renderDebugBboxes();

  // returns -1 if fails
  int addLight(const Point3 &pos, const Color3 &color, float radius, float attenuation_k = 1.f);
  int addLight(const Point3 &pos, const Color3 &color, float radius, const TMatrix &box, float attenuation_k = 1.f);
  int addLight(const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex, float attenuation_k = 1.f);
  int addLight(const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex, const TMatrix &box,
    float attenuation_k = 1.f);

  int addLight(const Light &l);

  void destroyLight(unsigned int id);

  void setLightPos(unsigned int id, const Point3 &pos);
  void setLightCol(unsigned int id, const Color3 &col);
  void setLightPosAndCol(unsigned int id, const Point3 &pos, const Color3 &color);
  void setLightRadius(unsigned int id, float radius);
  void setLightBox(unsigned int id, const TMatrix &box);
  void setLightDirection(unsigned int id, const Point3 &dir);

  void setLightTexture(unsigned int id, int tex);

  const Light &getLight(unsigned int id) const override;
  void setLight(unsigned int id, const Light &l);

  RenderOmniLight getRenderLight(unsigned int id) const override;

  vec4f getBoundingSphere(unsigned id) const override;

  void updateShadowVolume(uint32_t light_id) override;
};
