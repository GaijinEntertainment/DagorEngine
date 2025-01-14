//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "spotLight.h"
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_bitset.h>
#include <generic/dag_tabFwd.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <math/dag_hlsl_floatx.h>
#include "renderLights.hlsli"

#include <render/iesTextureManager.h>

struct Frustum;
class OmniShadowMap;
class Occlusion;

class SpotLightsManager
{
public:
  typedef SpotLight Light;
  typedef Light RawLight;
  typedef uint8_t mask_type_t;

  static constexpr int MAX_LIGHTS = 2048;
  enum
  {
    GI_LIGHT_MASK = 0x1, // TODO: maybe rename it
    MASK_LENS_FLARE = 0x2,
    MASK_ALL = 0xFF
  };

  SpotLightsManager();
  ~SpotLightsManager();

  void init();
  void close();

  template <bool use_small>
  void prepare(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *, bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane,
    const StaticTab<uint16_t, SpotLightsManager::MAX_LIGHTS> &shadow, float markSmallLightsAsFarLimit, vec3f cameraPos,
    mask_type_t accept_mask) const;

  void prepare(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *occ, bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane,
    const StaticTab<uint16_t, SpotLightsManager::MAX_LIGHTS> &shadow, float markSmallLightsAsFarLimit, vec3f cameraPos,
    mask_type_t accept_mask) const
  {
    prepare<true>(frustum, lights_inside_plane, lights_outside_plane, visibleIdBitset, occ, inside_box, outside_box, znear_plane,
      shadow, markSmallLightsAsFarLimit, cameraPos, accept_mask);
  }

  void prepare(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
    eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *occ, bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane,
    const StaticTab<uint16_t, SpotLightsManager::MAX_LIGHTS> &shadow, mask_type_t accept_mask)
  {
    prepare<false>(frustum, lights_inside_plane, lights_outside_plane, visibleIdBitset, occ, inside_box, outside_box, znear_plane,
      shadow, 0, v_zero(), accept_mask);
  }
  void renderDebugBboxes();
  int addLight(const Light &light); // return -1 if fails
  void destroyLight(unsigned int id);

  const Light &getLight(unsigned int id) const { return rawLights[id]; }
  void setLight(unsigned int id, const Light &l)
  {
    if (check_nan(l.pos_radius.x + l.pos_radius.y + l.pos_radius.z + l.pos_radius.w))
    {
      G_ASSERTF(0, "nan in setLight");
      return;
    }
    rawLights[id] = l;
    resetLightOptimization(id);
    updateBoundingSphere(id);
  }
  RenderSpotLight getRenderLight(unsigned int id) const
  {
    const Light &l = rawLights[id];
    const float cosInner = l.color_atten.a;
    const float cosOuter = cosHalfAngles[id];
    const float lightAngleScale = 1.0f / max(0.001f, (cosInner - cosOuter));
    const float lightAngleOffset = -cosOuter * lightAngleScale;
    RenderSpotLight ret;
    ret.lightPosRadius = (const float4 &)l.pos_radius;
    ret.lightColorAngleScale = (const float4 &)l.color_atten;
    ret.lightColorAngleScale.w = lightAngleScale;
    ret.lightDirectionAngleOffset = (const float4 &)l.dir_tanHalfAngle;
    ret.lightDirectionAngleOffset.w = lightAngleOffset;
    ret.texId_scale_shadow_contactshadow = (const float4 &)l.texId_scale;
    ret.texId_scale_shadow_contactshadow.z = l.shadows ? 1 : 0;
    ret.texId_scale_shadow_contactshadow.w = l.contactShadows ? 1 : 0;
    return ret;
  }

  void updateBoundingSphere(unsigned id)
  {
    const Light &l = rawLights[id];
    cosHalfAngles[id] = l.getCosHalfAngle();
    boundingSpheres[id] = l.getBoundingSphere(cosHalfAngles[id]);
    updateBoundingBox(id);
  }
  void updateBoundingBox(unsigned id);
  bbox3f getBoundingBox(unsigned id) const { return boundingBoxes[id]; }
  vec4f getBoundingSphere(unsigned id) const { return boundingSpheres[id]; }

  void destroyAllLights();

  int addLight(const Point3 &pos, const Color3 &color, const Point3 &dir, const float angle, float radius, float attenuation_k = 1.f,
    bool contact_shadows = false, int tex = -1);

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
    resetLightOptimization(id);
    updateBoundingSphere(id);
  }
  mask_type_t getLightMask(unsigned int id) const { return masks[id]; }
  void setLightMask(unsigned int id, mask_type_t mask) { masks[id] = mask; }
  Point3 getLightPos(unsigned int id) const { return Point3::xyz(rawLights[id].pos_radius); }
  Point4 getLightPosRadius(unsigned int id) const { return rawLights[id].pos_radius; }
  void getLightView(unsigned int id, mat44f &viewITM);
  void getLightPersp(unsigned int id, mat44f &proj);
  void setLightDirAngle(unsigned int id, const Point4 &dir_tanHalfAngle)
  {
    rawLights[id].dir_tanHalfAngle = dir_tanHalfAngle;
    resetLightOptimization(id);
    updateBoundingSphere(id);
  }
  const Point4 &getLightDirAngle(unsigned int id) const { return rawLights[id].dir_tanHalfAngle; }
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
    updateBoundingSphere(id);
  }
  void setLightRadius(unsigned int id, float radius)
  {
    if (check_nan(radius))
    {
      G_ASSERTF(0, "nan in setLightRadius");
      return;
    }
    rawLights[id].pos_radius.w = radius;
    resetLightOptimization(id);
    updateBoundingSphere(id);
  }
  void setLightCullingRadius(unsigned int id, float radius)
  {
    rawLights[id].culling_radius = radius;
    updateBoundingSphere(id);
  }
  void setLightShadows(unsigned int id, bool shadows) { rawLights[id].shadows = shadows; }

  void removeEmpty();
  int maxIndex() const { return maxLightIndex; }

  bool isLightNonOptimized(int id) { return nonOptLightIds.test(id); }
  bool tryGetNonOptimizedLightId(int &id)
  {
    if (int tId = nonOptLightIds.find_first(); tId != nonOptLightIds.kSize)
    {
      id = tId;
      return true;
    }
    return false;
  }
  void setLightOptimized(int id) { nonOptLightIds.set(id, false); }
  void resetLightOptimization(int id)
  {
    // Additional checks if optimization is needed can be added here
    bool shouldBeOptimized = (rawLights[id].pos_radius.w > 0) && (masks[id] & GI_LIGHT_MASK);
    nonOptLightIds.set(id, shouldBeOptimized);
    rawLights[id].culling_radius = -1.0f;
  }

  IesTextureCollection::PhotometryData getPhotometryData(int texId) const;

private:
  carray<Light, MAX_LIGHTS> rawLights;                 //-V730_NOINIT
  carray<vec4f, MAX_LIGHTS> boundingSpheres;           //-V730_NOINIT
  carray<bbox3f, MAX_LIGHTS> boundingBoxes;            //-V730_NOINIT
  alignas(16) carray<float, MAX_LIGHTS> cosHalfAngles; //-V730_NOINIT
  // masks allows to ignore specific lights in specific cases
  // for example, we can ignore highly dynamic lights for GI
  carray<mask_type_t, MAX_LIGHTS> masks; //-V730_NOINIT

  StaticTab<uint16_t, MAX_LIGHTS> freeLightIds; //-V730_NOINIT
  Bitset<MAX_LIGHTS> nonOptLightIds;
  IesTextureCollection *photometryTextures = nullptr;
  int maxLightIndex = -1;
};
