// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/spotLightsManager.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>
#include <generic/dag_sort.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug3d.h>
#include <math/dag_viewMatrix.h>
#include "smallLights.h"


SpotLightsManager::SpotLightsManager() : maxLightIndex(-1)
{
  G_STATIC_ASSERT(1ULL << (sizeof(*freeLightIds.data()) * 8) >= MAX_LIGHTS);
}


SpotLightsManager::~SpotLightsManager() { close(); }


void SpotLightsManager::init()
{
  mem_set_0(rawLights);
  mem_set_0(freeLightIds);
  freeLightIds.clear();
  nonOptLightIds.reset();
  photometryTextures = IesTextureCollection::acquireRef();
}

void SpotLightsManager::close()
{
  destroyAllLights();
  if (photometryTextures)
  {
    IesTextureCollection::releaseRef();
    photometryTextures = nullptr;
  }
}

typedef uint16_t shadow_index_t;

template <bool use_small>
void SpotLightsManager::prepare(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane, Tab<uint16_t> &lights_outside_plane,
  eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *occlusion, bbox3f &inside_box, bbox3f &outside_box, vec4f znear_plane,
  const StaticTab<shadow_index_t, SpotLightsManager::MAX_LIGHTS> &shadows, float markSmallLightsAsFarLimit, vec3f cameraPos,
  SpotLightMaskType require_any_mask) const
{
  v_bbox3_init_empty(inside_box);
  v_bbox3_init_empty(outside_box);
  const vec4f *boundings = (vec4f *)boundingSpheres.data();
  for (int i = 0; i <= maxLightIndex; ++i, boundings++)
  {
    if (require_any_mask && !(require_any_mask & masks[i]))
      continue;
    const RawLight &l = rawLights[i];
    if (l.pos_radius.w <= 0)
      continue;
    vec4f lightPosRad = *boundings;
    vec3f rad = v_splat_w(lightPosRad);
    if (!frustum.testSphereB(lightPosRad, rad)) // completely not visible
      continue;
    // if (occlusion && occlusion->isOccludedSphere(lightPosRad, rad))
    //   continue;
    if (occlusion && occlusion->isOccludedBox(boundingBoxes[i]))
      continue;
    if (visibleIdBitset)
      visibleIdBitset->set(i, true);

    vec4f res = v_add_x(v_sub_x(v_dot3_x(lightPosRad, znear_plane), rad), v_splat_w(znear_plane));
    vec4f length_sq = v_length3_sq(v_sub(cameraPos, lightPosRad));
    vec4f camInSphereVec = v_sub_x(length_sq, v_mul(rad, rad));

#if _TARGET_SIMD_SSE
    bool intersectsNear = _mm_movemask_ps(res) & 1;
    bool camInSphere = _mm_movemask_ps(camInSphereVec) & 1;
#else
    bool intersectsNear = v_test_vec_x_lt_0(res);
    bool camInSphere = v_test_vec_x_lt_0(camInSphereVec);
#endif
    const bool small = shadows[i] == shadow_index_t(~0) && is_viewed_small(lightPosRad, length_sq, markSmallLightsAsFarLimit);
    if ((intersectsNear || small) && !camInSphere)
    {
      inside_box.bmin = v_min(inside_box.bmin, v_sub(lightPosRad, rad));
      inside_box.bmax = v_max(inside_box.bmax, v_add(lightPosRad, rad));
      lights_inside_plane.push_back(i);
    }
    else
    {
      outside_box.bmin = v_min(outside_box.bmin, v_sub(lightPosRad, rad));
      outside_box.bmax = v_max(outside_box.bmax, v_add(lightPosRad, rad));
      lights_outside_plane.push_back(i);
    }
  }
}


int SpotLightsManager::addLight(const RawLight &light)
{
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
      logerr("Adding spotlight failed, already have %d lights in scene!", MAX_LIGHTS);
  }
  if (id < 0)
    return id;
  masks[id] = SpotLightMaskType::SPOT_LIGHT_MASK_DEFAULT;
  rawLights[id] = light;
  resetLightOptimization(id);
  updateBoundingSphere(id);
  return id;
}

struct AscCompare
{
  bool operator()(const uint16_t a, const uint16_t b) const { return a < b; }
};

void SpotLightsManager::removeEmpty()
{
  fast_sort(freeLightIds, AscCompare());
  for (int i = freeLightIds.size() - 1; i >= 0 && maxLightIndex == freeLightIds[i]; --i)
  {
    freeLightIds.pop_back();
    maxLightIndex--;
  }
}

void SpotLightsManager::renderDebugBboxes()
{
  begin_draw_cached_debug_lines();
  for (int i = 0; i <= maxLightIndex; ++i)
  {
    const RawLight &l = rawLights[i];
    if (l.pos_radius.w <= 0)
      continue;
    BBox3 box;
    v_stu_bbox3(box, boundingBoxes[i]);
    draw_cached_debug_box(box, E3DCOLOR(255, 0, 255, 255));
  }
  end_draw_cached_debug_lines();
}

void SpotLightsManager::destroyLight(unsigned int id)
{
  G_ASSERT_RETURN(id <= maxLightIndex, );

  setLightOptimized(id);
  memset(&rawLights[id], 0, sizeof(rawLights[id]));
  masks[id] = SpotLightMaskType::SPOT_LIGHT_MASK_NONE;

  if (id == maxLightIndex)
  {
    maxLightIndex--;
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


void SpotLightsManager::destroyAllLights()
{
  maxLightIndex = -1;
  freeLightIds.clear();
  nonOptLightIds.reset();
}

void SpotLightsManager::updateBoundingBox(unsigned id)
{
  const RawLight &l = rawLights[id];

  vec3f left, up;
  vec4f pos = v_ld(&l.pos_radius.x);
  float radius = l.culling_radius == -1 ? l.pos_radius.w : l.culling_radius;
  vec4f vrad = v_splats(radius);

  vec4f vdir = v_ld(&l.dir_tanHalfAngle.x);
  v_view_matrix_from_tangentZ(left, up, vdir);

  vec4f tanHalf = v_splat_w(vdir);
  vec4f sinHalfAngle = v_splat_x(v_div_x(tanHalf, v_sqrt_x(v_add_x(V_C_ONE, v_mul_x(tanHalf, tanHalf)))));
  vec4f mulR = v_mul(sinHalfAngle, vrad);
  static const bool buildOctahedron = true;
  if (buildOctahedron)
    mulR = v_mul(mulR, v_splats(1.082392200292394f)); // we build octahedron, so we have to scale radius by R/r
  left = v_mul(left, mulR);
  up = v_mul(up, mulR);

  bbox3f box;
  v_bbox3_init(box, left);

  if (buildOctahedron)
  {
    // v_bbox3_add_pt(box, left);//already inited
    v_bbox3_add_pt(box, up);
    v_bbox3_add_pt(box, v_neg(left));
    v_bbox3_add_pt(box, v_neg(up));
    left = v_mul(left, v_splats(0.7071067811865476f));
    up = v_mul(up, v_splats(0.7071067811865476f));
  }
  vec3f corner0 = v_add(left, up), corner1 = v_sub(left, up);
  v_bbox3_add_pt(box, corner0);
  v_bbox3_add_pt(box, v_neg(corner0));
  v_bbox3_add_pt(box, corner1);
  v_bbox3_add_pt(box, v_neg(corner1));
  vec3f farCenter = v_mul(vdir, vrad);
  box.bmin = v_add(box.bmin, farCenter);
  box.bmax = v_add(box.bmax, farCenter);
  v_bbox3_add_pt(box, v_zero());

  boundingBoxes[id].bmin = v_add(box.bmin, pos);
  boundingBoxes[id].bmax = v_add(box.bmax, pos);
}

void SpotLightsManager::getLightView(unsigned int id, mat44f &viewITM)
{
  TMatrix view;
  const Light &l = rawLights[id];
  view_matrix_from_tangentZ(Point3::xyz(l.dir_tanHalfAngle), view);
  view.setcol(3, Point3::xyz(l.pos_radius));
  v_mat44_make_from_43cu(viewITM, view[0]);
}

int SpotLightsManager::addLight(const Point3 &pos, const Color3 &color, const Point3 &dir, const float angle, float radius,
  float attenuation_k, bool contact_shadows, int tex, float illuminating_plane)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  return addLight(Light(pos, color, radius, attenuation_k, dir, angle, contact_shadows, false, tex, photometryData.zoom,
    photometryData.rotated, illuminating_plane));
}

IesTextureCollection::PhotometryData SpotLightsManager::getPhotometryData(int texId) const
{
  return photometryTextures->getTextureData(texId);
}

void SpotLightsManager::getLightPersp(unsigned int id, mat44f &proj)
{
  const Light &l = rawLights[id];
  float zn = 0.001f * l.pos_radius.w;
  float zf = l.pos_radius.w;
  float wk = 1. / l.dir_tanHalfAngle.w;

  v_mat44_make_persp_reverse(proj, wk, wk, zn, zf);
}
template void SpotLightsManager::prepare<true>(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane,
  Tab<uint16_t> &lights_outside_plane, eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *, bbox3f &inside_box,
  bbox3f &outside_box, vec4f znear_plane, const StaticTab<uint16_t, SpotLightsManager::MAX_LIGHTS> &shadow,
  float markSmallLightsAsFarLimit, vec3f cameraPos, SpotLightMaskType require_any_mask) const;

template void SpotLightsManager::prepare<false>(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane,
  Tab<uint16_t> &lights_outside_plane, eastl::bitset<MAX_LIGHTS> *visibleIdBitset, Occlusion *, bbox3f &inside_box,
  bbox3f &outside_box, vec4f znear_plane, const StaticTab<uint16_t, SpotLightsManager::MAX_LIGHTS> &shadow,
  float markSmallLightsAsFarLimit, vec3f cameraPos, SpotLightMaskType require_any_mask) const;
