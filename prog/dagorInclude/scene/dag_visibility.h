//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaders.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_frustum.h>


class VisibilityFinder
{
public:
  enum
  {
    OUTSIDE = Frustum::OUTSIDE,
    INSIDE = Frustum::INSIDE,
    INTERSECT = Frustum::INTERSECT,
    TOO_SMALL = 4,
    IS_OCCLUDED = 8,
  };
  static inline bool isInside(unsigned val) { return val == INSIDE; }
  static inline bool isIntersect(unsigned val) { return val == INTERSECT; }
  static inline bool isVisible(unsigned val) { return val & (INSIDE | INTERSECT); }
  static inline bool isInvisible(unsigned val) { return !(val & (INSIDE | INTERSECT)); }

  void set(vec4f viewpos, const Frustum &frustum_, float object_to_sceen_ratio, float near_ratio_offset,
    float visibility_range_multiplier, float hk, bool use_occlusion);

  // scalar implementation, to be obsoleted
  bool isScreenRatioVisible(const Point3 &sphc, float sph_radius_squared) const
  {
    vec4f r2 = v_splats(sph_radius_squared);
    return isScreenRatioVisible(v_ldu(&sphc.x), r2, r2);
  }
  bool isVisible(const BBox3 &box, const BSphere3 &c, float visrange) const;
  // vector implementation, to be used
  bool isScreenRatioVisible(vec3f sphc, vec4f sph_radius_squared, vec4f subobj_rad_sq) const;

  unsigned isVisibleBoxV(vec4f bmin_msoRadSq, vec4f bmax_sphRadSq) const;
  bool isVisibleBoxVLeaf(vec4f bmin_msoRadSq, vec4f bmax_sphRadSq) const;

  const Frustum &getFrustum() const { return frustum; }
  void setFrustum(const Frustum &f) { frustum = f; }
  void setUseOcclusion(bool use) { useOcclusion = use; }
  bool isUsingOcclusion() const { return useOcclusion; }
  vec3f getViewerPos() const { return viewerPos; }

protected:
  vec3f viewerPos = v_zero();
  vec4f c_nearRatioOffsetSq = v_zero(), c_squaredDistanceOffset = v_zero();
  Frustum frustum;
  bool useOcclusion = true;

  inline bool isScreenRatioVisibleInline(vec3f sphc, vec4f sphr2, vec4f msor_sq) const;
};

#include <supp/dag_define_COREIMP.h>

extern KRNLIMP VisibilityFinder *visibility_finder;

#include <supp/dag_undef_COREIMP.h>
