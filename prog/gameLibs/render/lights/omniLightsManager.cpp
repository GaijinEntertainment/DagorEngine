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
#include <EASTL/bit.h>

static bool are_approximately_equal(const OmniLight &a, const OmniLight &b)
{
  return are_approximately_equal(a.pos_radius, b.pos_radius, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.r, b.color_atten.r, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.g, b.color_atten.g, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.b, b.color_atten.b, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.a, b.color_atten.a, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.dir__tex_scale, b.dir__tex_scale, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.boxR0, b.boxR0, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.boxR1, b.boxR1, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.boxR2, b.boxR2, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.posRelToOrigin_cullRadius, b.posRelToOrigin_cullRadius, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.shadowZnZf_flags_sourceRadius, b.shadowZnZf_flags_sourceRadius, BaseLightsManager::FLOAT_EPS);
}

OmniLightMaskType &operator|=(OmniLightMaskType &lhs, OmniLightMaskType rhs)
{
  lhs = static_cast<OmniLightMaskType>(static_cast<eastl::underlying_type<OmniLightMaskType>::type>(lhs) | //-V1016
                                       static_cast<eastl::underlying_type<OmniLightMaskType>::type>(rhs)); //-V1016
  return lhs;
}

OmniLightsManager::OmniLightsManager(LightsManagerSharedStorage *shared_storage) :
  LightsManager("omni", shared_storage, SCENE_OMNI_MANAGED_LIGHTS_FIRST_INDEX * sizeof(ManagedLight),
    SCENE_OMNI_RENDER_LIGHTS_BYTE_OFFSET)
{}

template <>
bool LightsManager<OmniLight, RenderOmniLight, OmniLightMaskType, MAX_SCENE_OMNI_LIGHTS>::isInvalidatingShadowsNeed(
  const OmniLight &old_light, const OmniLight &new_light)
{
  return !are_approximately_equal(old_light.pos_radius, new_light.pos_radius, FLOAT_EPS) ||
         !are_approximately_equal(old_light.shadowZnZf_flags_sourceRadius, new_light.shadowZnZf_flags_sourceRadius, FLOAT_EPS) ||
         !are_approximately_equal(old_light.boxR0, new_light.boxR0, FLOAT_EPS) ||
         !are_approximately_equal(old_light.boxR1, new_light.boxR1, FLOAT_EPS) ||
         !are_approximately_equal(old_light.boxR2, new_light.boxR2, FLOAT_EPS);
}

void OmniLightsManager::drawDebugInfo()
{
  int maxIdx = maxIndex();
  for (int i = 0; i <= maxIdx; ++i)
  {
    if (!isLightValid(i))
      continue;

    const RawLight &l = rawLights[i];
    draw_debug_sph(Point3(l.pos_radius.x, l.pos_radius.y, l.pos_radius.z), l.pos_radius.w, e3dcolor(l.color_atten));
  }
}

void OmniLightsManager::renderDebugBboxes()
{
  begin_draw_cached_debug_lines();
  int maxIdx = maxIndex();
  for (int i = 0; i <= maxIdx; ++i)
  {
    if (!isLightValid(i))
      continue;

    const RawLight &l = rawLights[i];
    Point3 center = Point3::xyz(l.pos_radius);
    float radius = l.pos_radius.w;
    BBox3 box = BBox3(center - radius, center + radius);
    draw_cached_debug_box(box, E3DCOLOR(0, 255, 255, 255));
  }
  end_draw_cached_debug_lines();
}

int OmniLightsManager::addLight(const RawLight &l) { return allocateLight(l, OmniLightMaskType::OMNI_LIGHT_MASK_DEFAULT); }

void OmniLightsManager::destroyLight(unsigned int id) { deallocateLight(id); }

int OmniLightsManager::addLight(const Point3 &pos, const Color3 &color, float radius, float attenuation_k)
{
  return addLight(Light(pos, color, radius, attenuation_k));
}

int OmniLightsManager::addLight(const Point3 &pos, const Color3 &color, float radius, const TMatrix &box, float attenuation_k)
{
  return addLight(Light(pos, color, radius, attenuation_k, box));
}

int OmniLightsManager::addLight(const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex, float attenuation_k)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  return addLight(Light(pos, dir, color, radius, attenuation_k, tex, photometryData.zoom, photometryData.rotated));
}

int OmniLightsManager::addLight(const Point3 &pos, const Point3 &dir, const Color3 &color, float radius, int tex, const TMatrix &box,
  float attenuation_k)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  return addLight(Light(pos, dir, color, radius, attenuation_k, tex, photometryData.zoom, photometryData.rotated, box));
}

void OmniLightsManager::setLightPos(unsigned int id, const Point3 &pos)
{
  if (check_nan(pos.x + pos.y + pos.z))
  {
    G_ASSERTF(0, "nan in setLightPos");
    return;
  }
  if (are_approximately_equal(pos, Point3::xyz(rawLights[id].pos_radius), FLOAT_EPS))
    return;
  rawLights[id].pos_radius.x = pos.x;
  rawLights[id].pos_radius.y = pos.y;
  rawLights[id].pos_radius.z = pos.z;
  markRenderLightDirty(id);
}

void OmniLightsManager::setLightCol(unsigned int id, const Color3 &col)
{
  const Color4 &cur = rawLights[id].color_atten;
  if (are_approximately_equal(col.r, cur.r, FLOAT_EPS) && are_approximately_equal(col.g, cur.g, FLOAT_EPS) &&
      are_approximately_equal(col.b, cur.b, FLOAT_EPS))
    return;
  rawLights[id].color_atten.r = col.r;
  rawLights[id].color_atten.g = col.g;
  rawLights[id].color_atten.b = col.b;
  markRenderLightDirty(id);
}

void OmniLightsManager::setLightPosAndCol(unsigned int id, const Point3 &pos, const Color3 &color)
{
  setLightPos(id, pos);
  setLightCol(id, color);
}

void OmniLightsManager::setLightRadius(unsigned int id, float radius)
{
  if (check_nan(radius))
  {
    G_ASSERTF(0, "nan in setLightRadius");
    return;
  }
  if (are_approximately_equal(radius, 0.0f, FLOAT_EPS))
    radius = 0.0f;
  if (are_approximately_equal(radius, rawLights[id].pos_radius.w, FLOAT_EPS))
    return;
  rawLights[id].pos_radius.w = radius;
  markRenderLightDirty(id);
}

void OmniLightsManager::setLightBox(unsigned int id, const TMatrix &box)
{
  OmniLight updated = rawLights[id];
  updated.setBox(box);
  if (are_approximately_equal(rawLights[id].boxR0, updated.boxR0, FLOAT_EPS) &&
      are_approximately_equal(rawLights[id].boxR1, updated.boxR1, FLOAT_EPS) &&
      are_approximately_equal(rawLights[id].boxR2, updated.boxR2, FLOAT_EPS))
    return;
  rawLights[id].boxR0 = updated.boxR0;
  rawLights[id].boxR1 = updated.boxR1;
  rawLights[id].boxR2 = updated.boxR2;
  markRenderLightDirty(id);
}

void OmniLightsManager::setLightDirection(unsigned int id, const Point3 &dir)
{
  if (are_approximately_equal(dir, Point3::xyz(rawLights[id].dir__tex_scale), FLOAT_EPS))
    return;
  rawLights[id].setDirection(dir);
  markRenderLightDirty(id);
}

void OmniLightsManager::setLightTexture(unsigned int id, int tex)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  OmniLight updated = rawLights[id];
  updated.setTexture(tex, photometryData.zoom, photometryData.rotated);
  if (are_approximately_equal(rawLights[id].dir__tex_scale.w, updated.dir__tex_scale.w, FLOAT_EPS))
    return;
  rawLights[id].dir__tex_scale.w = updated.dir__tex_scale.w;
  markRenderLightDirty(id);
}

const OmniLight &OmniLightsManager::getLight(unsigned int id) const { return rawLights[id]; }

void OmniLightsManager::setLight(unsigned int id, const Light &l)
{
  if (check_nan(l.pos_radius.x + l.pos_radius.y + l.pos_radius.z + l.pos_radius.w))
  {
    G_ASSERTF(0, "nan in setLight");
    return;
  }
  float clampedRadius = l.pos_radius.w;
  if (are_approximately_equal(clampedRadius, 0.0f, FLOAT_EPS))
    clampedRadius = 0.0f;
  if (are_approximately_equal(rawLights[id], l) && are_approximately_equal(clampedRadius, rawLights[id].pos_radius.w, FLOAT_EPS))
    return;
  rawLights[id] = l;
  rawLights[id].pos_radius.w = clampedRadius;
  markRenderLightDirty(id);
}

RenderOmniLight OmniLightsManager::getRenderLight(unsigned int id) const
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
  ret.shadowZnZf_flags_packedDataBits = l.shadowZnZf_flags_sourceRadius;
  G_STATIC_ASSERT((OMNI_LIGHT_ID_MASK & (LIGHT_SOURCE_RADIUS_MASK << OMNI_LIGHT_SOURCE_RADIUS_BIT_OFFSET)) == 0);
  G_STATIC_ASSERT(MAX_SCENE_OMNI_LIGHTS <= OMNI_LIGHT_ID_MASK + 1);
  uint32_t sourceRadiusCm = pack_light_source_radius_cm(l.getSourceRadius());
  ret.shadowZnZf_flags_packedDataBits.w =
    eastl::bit_cast<float, uint32_t>((id & OMNI_LIGHT_ID_MASK) | (sourceRadiusCm << OMNI_LIGHT_SOURCE_RADIUS_BIT_OFFSET));
  return ret;
}

vec4f OmniLightsManager::getBoundingSphere(unsigned id) const
{
  const Light &l = rawLights[id];
  const float cullRadius = l.posRelToOrigin_cullRadius.w;
  vec4f bounds = v_ldu(reinterpret_cast<const float *>(&l.pos_radius.x));
  if (cullRadius > 0.f)
    bounds = v_perm_xyzd(bounds, v_splats(cullRadius));
  return bounds;
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
  if (l.shadowZnZf_flags_sourceRadius.x > 0)
    lightZnZfar.x = l.shadowZnZf_flags_sourceRadius.x;
  if (l.shadowZnZf_flags_sourceRadius.y > 0)
    lightZnZfar.y = l.shadowZnZf_flags_sourceRadius.y;

  vec3f vpos = v_make_vec4f(l.pos_radius.x, l.pos_radius.y, l.pos_radius.z, 0);
  shadowSystem->setOctahedralShadowVolume(shadowId, vpos, lightZnZfar.x, lightZnZfar.y, box);
}
