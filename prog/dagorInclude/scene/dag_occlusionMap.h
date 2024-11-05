//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_bounds3.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_staticTab.h>

class IGenLoad;
struct Frustum;
class Plane3;
class Point3;
class TMatrix;
class Occlusion;

class OcclusionMap
{
public:
  OcclusionMap() {}
  void load(IGenLoad &cb);
  void init(IGenLoad &cb) { load(cb); }
  bool prepare(Occlusion &occlusion); // return true if objects were rasterized
  void debugRender();
  void resetDynamicOccluders()
  {
    dynamicQuadOccluders.clear();
    dynamicBoxOccluders.clear();
  }

  inline int addOccluder(vec3f v0, vec3f v1, vec3f v2, const vec3f &v3)
  {
    if (dynamicQuadOccluders.size() == dynamicQuadOccluders.capacity())
      return -1;
    dynamicQuadOccluders.push_back(DynamicQuadOccluder(v0, v1, v2, v3));
    return dynamicQuadOccluders.size() - 1;
  }
  inline int addOccluder(mat44f_cref toBox, vec4f bmin, vec4f bmax)
  {
    if (dynamicBoxOccluders.size() == dynamicBoxOccluders.capacity())
      return -1;
    dynamicBoxOccluders.push_back(DynamicBoxOccluder(bmin, bmax, toBox));
    return dynamicBoxOccluders.size() - 1;
  }

protected:
  SmallTab<bbox3f, MidmemAlloc> boxOccludersBoxes;
  SmallTab<mat44f, MidmemAlloc> boxOccluders;
  SmallTab<float, MidmemAlloc> boxRadiusSq;

  SmallTab<mat44f, MidmemAlloc> quadOccluders;
  SmallTab<bbox3f, MidmemAlloc> quadOccludersBoxes;
  SmallTab<vec4f, MidmemAlloc> quadNormAndArea;
  float minQuadMetric = 1, minBoxMetric = 1;
  struct DynamicBoxOccluder
  {
    bbox3f box;
    mat44f tm;
    DynamicBoxOccluder() {} //-V730
    DynamicBoxOccluder(vec3f bmin_, vec3f bmax_, mat44f_cref tm_) : tm(tm_)
    {
      box.bmin = bmin_;
      box.bmax = bmax_;
    }
  };
  struct DynamicQuadOccluder
  {
    vec3f p0, p1, p2, p3;
    DynamicQuadOccluder() {} //-V730
    DynamicQuadOccluder(vec3f p0_, vec3f p1_, vec3f p2_, const vec3f &p3_) : p0(p0_), p1(p1_), p2(p2_), p3(p3_) {}
  };
  static constexpr int MAX_DYNAMIC_OCCLUDERS = 32;

  StaticTab<DynamicBoxOccluder, MAX_DYNAMIC_OCCLUDERS> dynamicBoxOccluders;
  StaticTab<DynamicQuadOccluder, MAX_DYNAMIC_OCCLUDERS> dynamicQuadOccluders;

public:
  struct BoxOccluder
  {
    float boxCorners_[8 * 3];
    float boxSpace_[4][3];

    Point3 *boxCorners() { return (Point3 *)(boxCorners_); }
    TMatrix &boxSpace() { return reinterpret_cast<TMatrix &>(boxSpace_); }
  };
  struct QuadOccluder
  {
    float corners_[4 * 3];
    float plane_[4];
    Point3 *corners() { return (Point3 *)(corners_); }
    Plane3 &plane() { return reinterpret_cast<Plane3 &>(plane_); }
  };
  struct OccluderInfo
  {
    BBox3 bbox;
    float radius2;
    enum
    {
      BOX = 0,
      QUAD = 1
    };
    int occType;
  };
  struct Occluder
  {
    union
    {
      QuadOccluder quad;
      BoxOccluder box;
    };
  };
};


#include <supp/dag_define_KRNLIMP.h>

extern KRNLIMP OcclusionMap *occlusion_map;

#include <supp/dag_undef_KRNLIMP.h>
