// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/spotLightsManager.h>
#include <math/dag_frustum.h>
#include <generic/dag_sort.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug3d.h>
#include <math/dag_viewMatrix.h>
#include <EASTL/bit.h>
#include <render/lights/shadowSystem.h>


static bool are_approximately_equal(const SpotLight &a, const SpotLight &b)
{
  return are_approximately_equal(a.pos_radius, b.pos_radius, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.culling_radius, b.culling_radius, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.r, b.color_atten.r, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.g, b.color_atten.g, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.b, b.color_atten.b, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.color_atten.a, b.color_atten.a, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.dir_tanHalfAngle, b.dir_tanHalfAngle, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.normalizedRollAngle, b.normalizedRollAngle, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.texId_scale_illuminatingPlane, b.texId_scale_illuminatingPlane, BaseLightsManager::FLOAT_EPS) &&
         a.shadows == b.shadows && a.contactShadows == b.contactShadows &&
         are_approximately_equal(a.getShadowTanHalfAngle(), b.getShadowTanHalfAngle(), BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.shadowFrustumOffset, b.shadowFrustumOffset, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.shadowNearFarClippingPlanes, b.shadowNearFarClippingPlanes, BaseLightsManager::FLOAT_EPS) &&
         are_approximately_equal(a.sourceRadius, b.sourceRadius, BaseLightsManager::FLOAT_EPS) &&
         a.requiresCullRadiusOptimization == b.requiresCullRadiusOptimization;
}

SpotLightMaskType &operator|=(SpotLightMaskType &lhs, SpotLightMaskType rhs)
{
  lhs = static_cast<SpotLightMaskType>(static_cast<eastl::underlying_type<SpotLightMaskType>::type>(lhs) | //-V1016
                                       static_cast<eastl::underlying_type<SpotLightMaskType>::type>(rhs)); //-V1016
  return lhs;
}

SpotLightsManager::SpotLightsManager(LightsManagerSharedStorage *shared_storage) :
  LightsManager("spot", shared_storage, SCENE_SPOT_MANAGED_LIGHTS_FIRST_INDEX * sizeof(ManagedLight),
    SCENE_SPOT_RENDER_LIGHTS_BYTE_OFFSET)
{
  mem_set_0(boundingSpheres);
  mem_set_0(boundingBoxes);
  mem_set_0(cosHalfAngles);
}

template <>
bool LightsManager<SpotLight, RenderSpotLight, SpotLightMaskType, MAX_SCENE_SPOT_LIGHTS>::isInvalidatingShadowsNeed(
  const SpotLight &old_light, const SpotLight &new_light)
{
  return !are_approximately_equal(old_light.pos_radius, new_light.pos_radius, FLOAT_EPS) ||
         !are_approximately_equal(old_light.shadowNearFarClippingPlanes, new_light.shadowNearFarClippingPlanes, FLOAT_EPS) ||
         !are_approximately_equal(old_light.texId_scale_illuminatingPlane.z, new_light.texId_scale_illuminatingPlane.z, FLOAT_EPS) ||
         !are_approximately_equal(old_light.shadowFrustumOffset, new_light.shadowFrustumOffset, FLOAT_EPS) ||
         !are_approximately_equal(old_light.getShadowTanHalfAngle(), new_light.getShadowTanHalfAngle(), FLOAT_EPS) ||
         !are_approximately_equal(old_light.dir_tanHalfAngle, new_light.dir_tanHalfAngle, FLOAT_EPS);
}

int SpotLightsManager::addLight(const RawLight &light) { return allocateLight(light, SpotLightMaskType::SPOT_LIGHT_MASK_DEFAULT); }

const SpotLight &SpotLightsManager::getLight(unsigned int id) const { return rawLights[id]; }

void SpotLightsManager::setLight(unsigned int id, const Light &l)
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
  // reset optimization only when optimization related parameters changed
  bool resetOptimization = !are_approximately_equal(rawLights[id].pos_radius, l.pos_radius, FLOAT_EPS) ||
                           !are_approximately_equal(rawLights[id].dir_tanHalfAngle, l.dir_tanHalfAngle, FLOAT_EPS);
  rawLights[id] = l;
  rawLights[id].pos_radius.w = clampedRadius;
  if (resetOptimization)
    resetLightOptimization(id);
  updateBoundingSphere(id);
  markRenderLightDirty(id);
}

void SpotLightsManager::afterLightAllocation(unsigned int id)
{
  LightsManager::afterLightAllocation(id);
  resetLightOptimization(id);
  updateBoundingSphere(id);
}

RenderSpotLight SpotLightsManager::getRenderLight(unsigned int id) const
{
  const Light &l = rawLights[id];
  const float cosInner = l.color_atten.a;
  const float cosOuter = cosHalfAngles[id];
  const float lightAngleScale = 1.0f / max(0.001f, (cosInner - cosOuter));
  const float lightAngleOffset = -cosOuter * lightAngleScale;
  const float rollAngle = l.normalizedRollAngle;
  RenderSpotLight ret;
  ret.lightPos_halfRadius_halfCullRadius.x = l.pos_radius.x;
  ret.lightPos_halfRadius_halfCullRadius.y = l.pos_radius.y;
  ret.lightPos_halfRadius_halfCullRadius.z = l.pos_radius.z;
  uint32_t packedRadiuses =
    (static_cast<uint32_t>(float_to_half(l.culling_radius)) << 16) | static_cast<uint32_t>(float_to_half(l.pos_radius.w));
  ret.lightPos_halfRadius_halfCullRadius.w = eastl::bit_cast<float>(packedRadiuses);
  ret.lightColorAngleScale = (const float4 &)l.color_atten;
  ret.lightColorAngleScale.w = lightAngleScale;
  ret.lightDirectionAngleOffset = (const float4 &)l.dir_tanHalfAngle;
  ret.lightDirectionAngleOffset.w = lightAngleOffset;
  ret.texId_scale_illuminatingplane_packedDataBits = (const float4 &)l.texId_scale_illuminatingPlane;
  if (ret.texId_scale_illuminatingplane_packedDataBits.y == 0)
  {
    // Used for projected textures
    const float halfAngleTan = l.dir_tanHalfAngle.w;
    const float invHalfAngleSin = safediv(sqrtf(1.f + halfAngleTan * halfAngleTan), halfAngleTan);
    ret.texId_scale_illuminatingplane_packedDataBits.y = invHalfAngleSin;
  }
  uint32_t packedRollAngle = static_cast<uint32_t>(float(SPOT_LIGHT_ROLL_MAX_VALUE) * rollAngle) << SPOT_LIGHT_ROLL_BIT_OFFSET;
  uint32_t shadowFlags = (l.contactShadows ? SPOT_LIGHT_NEEDS_CONTACT_SHADOWS_MASK : 0) | (l.shadows ? SPOT_LIGHT_HAS_SHADOW_MASK : 0);
  G_STATIC_ASSERT((SPOT_LIGHT_NEEDS_CONTACT_SHADOWS_MASK | SPOT_LIGHT_HAS_SHADOW_MASK) == SPOT_LIGHT_CONTACT_SHADOW_MASK);
  G_STATIC_ASSERT((SPOT_LIGHT_CONTACT_SHADOW_MASK & SPOT_LIGHT_ROLL_MASK) == 0);
  G_ASSERT((shadowFlags & SPOT_LIGHT_CONTACT_SHADOW_MASK) == shadowFlags);
  G_ASSERT((packedRollAngle & SPOT_LIGHT_ROLL_MASK) == packedRollAngle);
  G_STATIC_ASSERT((SPOT_LIGHT_ID_MASK & (SPOT_LIGHT_ROLL_MASK | SPOT_LIGHT_CONTACT_SHADOW_MASK)) == 0);
  G_STATIC_ASSERT(MAX_SCENE_SPOT_LIGHTS <= (1 << SPOT_LIGHT_ID_BIT_COUNT));
  uint32_t packedId = uint32_t(id) << SPOT_LIGHT_ID_BIT_OFFSET;
  uint32_t packedSourceRadius = pack_light_source_radius_cm(l.sourceRadius) << SPOT_LIGHT_SOURCE_RADIUS_BIT_OFFSET;
  ret.texId_scale_illuminatingplane_packedDataBits.w =
    eastl::bit_cast<float, uint32_t>(packedRollAngle | shadowFlags | packedId | packedSourceRadius);

  return ret;
}

void SpotLightsManager::renderDebugBboxes()
{
  begin_draw_cached_debug_lines();
  int maxIdx = maxIndex();
  for (int i = 0; i <= maxIdx; ++i)
  {
    if (!isLightValid(i))
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

void SpotLightsManager::updateBoundingSphere(unsigned id)
{
  const Light &l = rawLights[id];
  cosHalfAngles[id] = l.getCosHalfAngle();
  boundingSpheres[id] = l.getBoundingSphere(cosHalfAngles[id]);
  updateBoundingBox(id);
}

bbox3f SpotLightsManager::getBoundingBox(unsigned id) const { return boundingBoxes[id]; }

vec4f SpotLightsManager::getBoundingSphere(unsigned id) const { return boundingSpheres[id]; }

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

void SpotLightsManager::setLightPos(unsigned int id, const Point3 &pos)
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
  resetLightOptimization(id);
  updateBoundingSphere(id);
  markRenderLightDirty(id);
}

Point3 SpotLightsManager::getLightPos(unsigned int id) const { return Point3::xyz(rawLights[id].pos_radius); }

Point4 SpotLightsManager::getLightPosRadius(unsigned int id) const { return rawLights[id].pos_radius; }

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

void SpotLightsManager::setLightDirAngle(unsigned int id, const Point4 &dir_tanHalfAngle, const Point3 &light_up_dir)
{
  const float newRollAngle = SpotLight::get_normalized_roll_angle(Point3::xyz(dir_tanHalfAngle), light_up_dir);
  if (are_approximately_equal(dir_tanHalfAngle, rawLights[id].dir_tanHalfAngle, FLOAT_EPS) &&
      are_approximately_equal(newRollAngle, rawLights[id].normalizedRollAngle, FLOAT_EPS))
    return;
  rawLights[id].dir_tanHalfAngle = dir_tanHalfAngle;
  rawLights[id].normalizedRollAngle = newRollAngle;
  resetLightOptimization(id);
  updateBoundingSphere(id);
  markRenderLightDirty(id);
}

const Point4 &SpotLightsManager::getLightDirAngle(unsigned int id) const { return rawLights[id].dir_tanHalfAngle; }

void SpotLightsManager::setLightCol(unsigned int id, const Color3 &col)
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

void SpotLightsManager::setLightPosAndCol(unsigned int id, const Point3 &pos, const Color3 &color)
{
  setLightPos(id, pos);
  setLightCol(id, color);
}

void SpotLightsManager::setLightRadius(unsigned int id, float radius)
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
  resetLightOptimization(id);
  updateBoundingSphere(id);
  markRenderLightDirty(id);
}

void SpotLightsManager::setLightCullingRadius(unsigned int id, float radius)
{
  if (are_approximately_equal(radius, rawLights[id].culling_radius, FLOAT_EPS))
    return;
  rawLights[id].culling_radius = radius;
  updateBoundingSphere(id);
  markRenderLightDirty(id);
}

void SpotLightsManager::setLightShadows(unsigned int id, bool shadows)
{
  if (rawLights[id].shadows == shadows)
    return;
  rawLights[id].shadows = shadows;
  markRenderLightDirty(id);
}

bool SpotLightsManager::isLightNonOptimized(int id)
{
  OSSpinlockScopedLock lock(nonOptLightIdsLock);
  return nonOptLightIds.test(id);
}

bool SpotLightsManager::tryGetNonOptimizedLightId(int &id)
{
  OSSpinlockScopedLock lock(nonOptLightIdsLock);
  if (int tId = nonOptLightIds.find_first(); tId != nonOptLightIds.kSize)
  {
    id = tId;
    return true;
  }
  return false;
}

void SpotLightsManager::setLightOptimized(int id)
{
  OSSpinlockScopedLock lock(nonOptLightIdsLock);
  nonOptLightIds.set(id, false);
}

void SpotLightsManager::resetLightOptimization(int id)
{
  // Additional checks if optimization is needed can be added here
  bool shouldBeOptimized = (isLightValid(id)) && (masks[id] & SpotLightMaskType::SPOT_LIGHT_MASK_GI);
  {
    OSSpinlockScopedLock lock(nonOptLightIdsLock);
    nonOptLightIds.set(id, shouldBeOptimized);
  }
  rawLights[id].culling_radius = -1.0f;
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