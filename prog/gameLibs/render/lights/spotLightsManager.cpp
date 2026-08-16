// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/spotLightsManager.h>
#include <math/dag_frustum.h>
#include <generic/dag_sort.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug3d.h>
#include <math/dag_viewMatrix.h>
#include <render/lights/shadowSystem.h>


SpotLightsManager::SpotLightsManager(const char *name) : LightsManager(name)
{
  mem_set_0(boundingSpheres);
  mem_set_0(boundingBoxes);
  mem_set_0(cosHalfAngles);
  nonOptLightIds.reset();
}

SpotLightsManager::SpotLightsManager() : SpotLightsManager("spot") {}

int SpotLightsManager::addLight(const RawLight &light) { return allocateLight(light, SpotLightMaskType::SPOT_LIGHT_MASK_DEFAULT); }

void SpotLightsManager::afterLightAllocation(unsigned int id)
{
  LightsManager::afterLightAllocation(id);
  resetLightOptimization(id);
  updateBoundingSphere(id);
}

void SpotLightsManager::renderDebugBboxes()
{
  begin_draw_cached_debug_lines();
  int maxIdx = maxIndex();
  for (int i = 0; i <= maxIdx; ++i)
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

void SpotLightsManager::destroyLight(unsigned int id) { deallocateLight(id); }

void SpotLightsManager::beforeLightDeallocation(unsigned int id)
{
  LightsManager::beforeLightDeallocation(id);
  setLightOptimized(id);
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
  view.setcol(3,
    Point3::xyz(l.pos_radius) +
      Point3::xyz(l.dir_tanHalfAngle) * (l.shadowFrustumOffset - l.shadowNearFarClippingPlanes.x - l.texId_scale_illuminatingPlane.z));
  v_mat44_make_from_43cu(viewITM, view[0]);
}

int SpotLightsManager::addLight(const Point3 &pos, const Color3 &color, const Point3 &dir, const float angle, float radius,
  float attenuation_k, bool contact_shadows, const Point3 &light_up_dir, int tex, float illuminating_plane)
{
  IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
  return addLight(Light(pos, color, radius, attenuation_k, dir, light_up_dir, angle, contact_shadows, false, tex, photometryData.zoom,
    photometryData.rotated, illuminating_plane));
}

void SpotLightsManager::getLightPersp(unsigned int id, mat44f &proj)
{
  const Light &l = rawLights[id];
  Point2 lightZnZfar = get_light_shadow_zn_zf(l.pos_radius.w);
  float zn = max(lightZnZfar.x, l.shadowNearFarClippingPlanes.x + l.texId_scale_illuminatingPlane.z);
  float zf = l.shadowNearFarClippingPlanes.y > 0.f ? l.shadowNearFarClippingPlanes.y : lightZnZfar.y;
  float wk = 1.f / l.getShadowTanHalfAngle();

  v_mat44_make_persp_reverse(proj, wk, wk, zn, zf);
}

static inline float area_spotlight_zn(float default_zn, float illuminating_plane_offset)
{
  return max(default_zn, illuminating_plane_offset);
}

void SpotLightsManager::updateShadowVolume(uint32_t light_id)
{
  const auto shadowId = getShadowId(light_id);
  if (shadowId == INVALID_SHADOW_VOLUME_ID)
  {
    return;
  }

  const auto &l = getLight(light_id);
  mat44f viewITM;
  getLightView(light_id, viewITM);

  bbox3f box;
  v_bbox3_init_empty(box);
  float2 lightZnZfar = get_light_shadow_zn_zf(l.pos_radius.w);
  lightZnZfar.x = area_spotlight_zn(lightZnZfar.x, l.shadowNearFarClippingPlanes.x + l.texId_scale_illuminatingPlane.z);
  lightZnZfar.y = l.shadowNearFarClippingPlanes.y > 0.f ? l.shadowNearFarClippingPlanes.y : lightZnZfar.y;
  shadowSystem->setShadowVolume(shadowId, viewITM, lightZnZfar.x, lightZnZfar.y, 1. / l.getShadowTanHalfAngle(), box);
}