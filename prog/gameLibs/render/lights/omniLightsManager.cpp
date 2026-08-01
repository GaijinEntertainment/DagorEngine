// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/omniLightsManager.h>
#include <math/dag_frustum.h>
#include <generic/dag_sort.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug3d.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <util/dag_texMetaData.h>
#include <drv/3d/dag_driver.h>
#include <render/lights/shadowSystem.h>
#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>

OmniLightsManager::OmniLightsManager() : LightsManager<OmniLight>("omni")
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
  masks[id] = OmniLightMaskType::OMNI_LIGHT_MASK_DEFAULT;
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
  masks[id] = OmniLightMaskType::OMNI_LIGHT_MASK_NONE;

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

void OmniLightsManager::updateShadowVolume(uint32_t light_id)
{
  const auto shadowId = getShadowId(light_id);
  if (shadowId == INVALID_SHADOW_VOLUME_ID)
  {
    return;
  }
  const auto &l = getLight(light_id);

  bbox3f box;
  v_bbox3_init_empty(box);
  float2 lightZnZfar = get_light_shadow_zn_zf(l.pos_radius.w);
  if (l.shadowNearFarClippingPlanesPad.x > 0)
    lightZnZfar.x = l.shadowNearFarClippingPlanesPad.x;
  if (l.shadowNearFarClippingPlanesPad.y > 0)
    lightZnZfar.y = l.shadowNearFarClippingPlanesPad.y;

  vec3f vpos = v_make_vec4f(l.pos_radius.x, l.pos_radius.y, l.pos_radius.z, 0);
  shadowSystem->setOctahedralShadowVolume(shadowId, vpos, lightZnZfar.x, lightZnZfar.y, box);
}
