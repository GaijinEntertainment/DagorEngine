// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <scene/dag_visibility.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_materialData.h>
#include <workCycle/dag_gameSettings.h>
#include <generic/dag_tabUtils.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <math/dag_mathUtils.h>

void VisibilityFinder::set(vec4f viewpos, const Frustum &frustum_, float object_to_sceen_ratio, float near_ratio_offset,
  float visibility_range_multiplier, float hk, const Occlusion *occlusion_)
{

  frustum = frustum_;
  viewerPos = viewpos;
  float squaredDistanceOffset = (object_to_sceen_ratio / hk) / (visibility_range_multiplier);
  squaredDistanceOffset *= squaredDistanceOffset;
  float nearRatioOffsetSq = near_ratio_offset * near_ratio_offset;
  c_squaredDistanceOffset = v_splats(squaredDistanceOffset);
  c_nearRatioOffsetSq = v_splats(nearRatioOffsetSq);
  occlusion = occlusion_;
}

__forceinline bool VisibilityFinder::isScreenRatioVisibleInline(vec3f sphc, vec4f sphr2, vec4f msor_sq) const
{
  vec4f distSq = v_length3_sq_x(v_sub(sphc, viewerPos));
  vec4f sq_dif = v_sub_x(distSq, sphr2);
  vec4f distSqOfs = v_mul_x(sq_dif, c_squaredDistanceOffset);

  if (v_test_vec_x_ge(msor_sq, v_min(distSq, distSqOfs)))
    return true;

  vec4f dist1 = v_add_x(v_sqrt_fast_x(distSq), V_C_ONE);
  vec4f nearComparsion = v_div_x(sq_dif, v_sub_x(v_mul(dist1, dist1), sphr2));
  return v_test_vec_x_le(nearComparsion, c_nearRatioOffsetSq) ? true : false;
}

bool VisibilityFinder::isScreenRatioVisible(vec3f sphc, vec4f sphr2, vec4f subobj_rad_sq) const
{
  return isScreenRatioVisibleInline(sphc, sphr2, subobj_rad_sq);
}

bool VisibilityFinder::isVisibleBoxVLeaf(vec4f bmin_msoRadSq, vec4f bmax_sphRadSq) const
{
  vec4f center2 = v_add(bmax_sphRadSq, bmin_msoRadSq);
  vec4f extent2 = v_sub(bmax_sphRadSq, bmin_msoRadSq);
  vec4f center = v_mul(center2, V_C_HALF);

  // Visibility range.
  if (!isScreenRatioVisibleInline(center, v_splat_w(bmax_sphRadSq), v_splat_w(bmin_msoRadSq)))
    return false;

  // Box vs. frustum.
  if (!frustum.testBoxExtentB(center2, extent2))
    return false;


  // Box vs. occlusion map.

  if (occlusion && occlusion->isOccludedBoxExtent2(center2, extent2))
    return false;
  return true;
}

unsigned VisibilityFinder::isVisibleBoxV(vec4f bmin_msoRadSq, vec4f bmax_sphRadSq) const
{

  vec4f center2 = v_add(bmax_sphRadSq, bmin_msoRadSq);
  vec4f extent2 = v_sub(bmax_sphRadSq, bmin_msoRadSq);
  vec4f center = v_mul(center2, V_C_HALF);

  // Visibility range.
  if (!isScreenRatioVisibleInline(center, v_splat_w(bmax_sphRadSq), v_splat_w(bmin_msoRadSq)))
    return TOO_SMALL;

  // Box vs. frustum.
  unsigned intersect = frustum.testBoxExtent(center2, extent2);
  if (intersect == Frustum::OUTSIDE)
    return OUTSIDE;


  // Box vs. occlusion map.

  if (occlusion && occlusion->isOccludedBoxExtent2(center2, extent2))
    return IS_OCCLUDED;
  return intersect;
}

bool VisibilityFinder::isVisible(const BBox3 &box_s, const BSphere3 &sph, float visrange) const
{
  bbox3f box = v_ldu_bbox3(box_s);
  vec4f center2 = v_add(box.bmax, box.bmin);
  vec4f extent2 = v_sub(box.bmax, box.bmin);
  vec4f sphc = v_ldu(&sph.c.x);

  // Visibility range.
  if (visrange > 0.f)
  {
    float distSq = v_extract_x(v_length3_sq_x(v_sub(sphc, viewerPos)));
    float visibilityRange = sph.r + visrange;
    if (distSq > visibilityRange * visibilityRange)
      return false;
    return true;
  }
  else
  {
    vec4f sphr2 = v_splats(sph.r2);
    if (!isScreenRatioVisibleInline(sphc, sphr2, sphr2))
      return false;
  }
  // Box vs. frustum.
  //
  if (!frustum.testBoxExtentB(center2, extent2))
    return false;


  // Box vs. occlusion map.

  if (occlusion && occlusion->isOccludedBoxExtent2(center2, extent2))
    return false;
  return true;
}
