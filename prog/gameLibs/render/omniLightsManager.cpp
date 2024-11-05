// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/omniLightsManager.h>
#include <math/dag_frustum.h>
#include <generic/dag_sort.h>
#include <scene/dag_occlusion.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug3d.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <util/dag_texMetaData.h>
#include <drv/3d/dag_driver.h>
#include "smallLights.h"

#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>

OmniLightsManager::OmniLightsManager()
{
  G_STATIC_ASSERT(1ULL << (sizeof(*freeLightIds.data()) * 8) >= MAX_LIGHTS);

  mem_set_0(rawLights);
  mem_set_0(freeLightIds);
  mem_set_0(lightPriority);
  freeLightIds.clear();

  photometryTextures = IesTextureCollection::acquireRef();
}


OmniLightsManager::~OmniLightsManager() { close(); }

void OmniLightsManager::close()
{
  destroyAllLights();
  if (photometryTextures)
  {
    IesTextureCollection::releaseRef();
    photometryTextures = nullptr;
  }
}

void OmniLightsManager::prepare(const Frustum &frustum, Tab<uint16_t> &lights_with_camera_inside,
  Tab<uint16_t> &lights_with_camera_outside, Occlusion *occlusion, bbox3f &inside_box, bbox3f &outside_box,
  const StaticTab<uint16_t, MAX_LIGHTS> &shadow, float markSmallLightsAsFarLimit, vec3f cameraPos, mask_type_t accept_mask) const
{
  prepare(frustum, lights_with_camera_inside, lights_with_camera_outside, occlusion, inside_box, outside_box, frustum.camPlanes[5],
    shadow, markSmallLightsAsFarLimit, cameraPos, accept_mask);
}

void OmniLightsManager::prepare(const Frustum &frustum, Tab<uint16_t> &lightsInside, Tab<uint16_t> &lightsOutside,
  Occlusion *occlusion, bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane, const StaticTab<uint16_t, MAX_LIGHTS> &shadow,
  float markSmallLightsAsFarLimit, vec3f cameraPos, mask_type_t accept_mask) const
{
  prepare(frustum, lightsInside, lightsOutside, nullptr, occlusion, inside_box, outside_box, znear_plane, shadow,
    markSmallLightsAsFarLimit, cameraPos, accept_mask);
}

typedef uint16_t shadow_index_t;

void OmniLightsManager::prepare(const Frustum &frustum, Tab<uint16_t> &lightsInside, Tab<uint16_t> &lightsOutside,
  eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *occlusion, bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane,
  const StaticTab<uint16_t, MAX_LIGHTS> &shadows, float markSmallLightsAsFarLimit, vec3f cameraPos, mask_type_t accept_mask) const
{
  int maxIdx = maxIndex();
  int reserveSize = (maxIdx + 1) / 2;
  lightsInside.reserve(reserveSize);
  lightsOutside.reserve(reserveSize);
  vec4f rad_scale = v_splats(1.1f);
  for (int i = 0; i <= maxIdx; ++i)
  {
    if (!(accept_mask & masks[i]))
      continue;
    const RawLight &l = rawLights[i];
    if (l.pos_radius.w <= 0)
      continue;
    vec4f lightPosRad = v_ld(&l.pos_radius.x);
    vec3f rad = v_splat_w(lightPosRad);
    if (!frustum.testSphereB(lightPosRad, rad))
      continue;
    if (occlusion && occlusion->isOccludedSphere(lightPosRad, rad))
      continue;
    if (visibleIdBitset)
      visibleIdBitset->set(i, true);

    vec4f radScaled = v_mul_x(rad_scale, rad);
    vec4f res = v_add_x(v_sub_x(v_dot3_x(lightPosRad, znear_plane), radScaled), v_splat_w(znear_plane));
    vec4f length_sq = v_length3_sq(v_sub(cameraPos, lightPosRad));
    vec4f camInSphereVec = v_sub_x(length_sq, v_mul(rad, rad));

#if _TARGET_SIMD_SSE
    bool intersectsNear = _mm_movemask_ps(res) & 1;
    bool camInSphere = _mm_movemask_ps(camInSphereVec) & 1;
#else
    bool intersectsNear = v_test_vec_x_lt_0(res);
    bool camInSphere = v_test_vec_x_lt_0(camInSphereVec);
#endif
    bool small = shadows[i] == shadow_index_t(~0) && is_viewed_small(lightPosRad, length_sq, markSmallLightsAsFarLimit);
    if ((intersectsNear || small) && !camInSphere)
    {
      inside_box.bmin = v_min(inside_box.bmin, v_sub(lightPosRad, rad));
      inside_box.bmax = v_max(inside_box.bmax, v_add(lightPosRad, rad));
      lightsInside.push_back(i);
    }
    else
    {
      outside_box.bmin = v_min(outside_box.bmin, v_sub(lightPosRad, rad));
      outside_box.bmax = v_max(outside_box.bmax, v_add(lightPosRad, rad));
      lightsOutside.push_back(i);
    }
  }
}


void OmniLightsManager::drawDebugInfo()
{
  int maxIdx = maxIndex();
  for (int i = 0; i <= maxIdx; ++i)
  {
    const RawLight &l = rawLights[i];
    if (l.pos_radius.w <= 0)
      continue;
    draw_debug_sph(Point3(l.pos_radius.x, l.pos_radius.y, l.pos_radius.z), l.pos_radius.w, e3dcolor(l.color_atten));
  }
}

void OmniLightsManager::renderDebugBboxes()
{
  begin_draw_cached_debug_lines();
  int maxIdx = maxIndex();
  for (int i = 0; i <= maxIdx; ++i)
  {
    const RawLight &l = rawLights[i];
    if (l.pos_radius.w <= 0)
      continue;
    Point3 center = Point3::xyz(l.pos_radius);
    float radius = l.pos_radius.w;
    BBox3 box = BBox3(center - radius, center + radius);
    draw_cached_debug_box(box, E3DCOLOR(0, 255, 255, 255));
  }
  end_draw_cached_debug_lines();
}

int OmniLightsManager::addLight(int priority, const RawLight &l)
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
      logerr("Adding omnilight failed, already have %d lights in scene!", MAX_LIGHTS);
  }
  if (id < 0)
    return id;
  rawLights[id] = l;
  masks[id] = ~mask_type_t(0);
  lightPriority[id] = priority;
  return id;
}

struct AscCompare
{
  bool operator()(const uint16_t a, const uint16_t b) const { return a < b; }
};

void OmniLightsManager::destroyLight(unsigned int id)
{
  OSSpinlockScopedLock lock(lightAllocationSpinlock);
  G_ASSERT_RETURN(id <= maxLightIndex, );

  memset(&rawLights[id], 0, sizeof(rawLights[id]));
  masks[id] = 0;

  if (id == maxLightIndex)
  {
    --maxLightIndex;
    return;
  }

#if DAGOR_DBGLEVEL > 0
  for (int i = 0; i < freeLightIds.size(); ++i)
    if (freeLightIds[i] == id)
    {
      G_ASSERTF(freeLightIds[i] != id, "Light %d is already destroyed, re-destroy is invalid", id);
      return;
    }
#endif
  freeLightIds.push_back(id);
}


void OmniLightsManager::destroyAllLights()
{
  OSSpinlockScopedLock lock(lightAllocationSpinlock);
  maxLightIndex = -1;
  freeLightIds.clear();
}

IesTextureCollection::PhotometryData OmniLightsManager::getPhotometryData(int texId) const
{
  return photometryTextures->getTextureData(texId);
}

int OmniLightsManager::addLight(int priority, const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex,
  float attenuation_k)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  return addLight(priority, Light(pos, dir, color, radius, attenuation_k, tex, photometryData.zoom, photometryData.rotated));
}

int OmniLightsManager::addLight(int priority, const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex,
  const TMatrix &box, float attenuation_k)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  return addLight(priority, Light(pos, dir, color, radius, attenuation_k, tex, photometryData.zoom, photometryData.rotated, box));
}

void OmniLightsManager::setLightTexture(unsigned int id, int tex)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  rawLights[id].setTexture(tex, photometryData.zoom, photometryData.rotated);
}
