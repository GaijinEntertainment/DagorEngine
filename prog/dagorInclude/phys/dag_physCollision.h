//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_plane3.h>
class HeightmapHandler;
class LandMeshHolesManager;

class PhysCollision
{
protected:
  enum TYPE
  {
    TYPE_EMPTY,
    TYPE_COMPOUND,
    TYPE_SPHERE,
    TYPE_BOX,
    TYPE_CAPSULE,
    TYPE_CYLINDER,
    TYPE_PLANE,
    TYPE_HEIGHTFIELD,
    TYPE_TRIMESH,
    TYPE_CONVEXHULL,

    TYPE_NATIVE_SHAPE = ~0u
  };

public:
  PhysCollision(TYPE t) : collType(t) {}
  void setMargin(float m) { collMargin = m; }
  float getMargin() const { return collMargin; }

  static int detectDirAxisIndex(TMatrix &inout_tm)
  {
    int col = 0;
    float diff = fabsf(inout_tm.getcol(0).lengthSq() - 1.0f);
    if (diff + fabsf(fabsf(inout_tm.getcol(0)[0]) - 1.0f) < 1e-3)
      col = 0;
    else if (diff + fabsf(fabsf(inout_tm.getcol(0)[1]) - 1.0f) < 1e-3)
      col = 1;
    else if (diff + fabsf(fabsf(inout_tm.getcol(0)[2]) - 1.0f) < 1e-3)
      col = 2;
    else
      return 0;

    inout_tm.setcol(0, Point3(1, 0, 0));
    inout_tm.setcol(1, Point3(0, 1, 0));
    inout_tm.setcol(2, Point3(0, 0, 1));
    return col;
  }
  static bool normalizeBox(Point3 &inout_box_extents, TMatrix &inout_tm)
  {
    if (memcmp(&inout_tm, &TMatrix::IDENT, sizeof(Point3) * 3) == 0) //-V1014 //-V512
      return false;
    unsigned zeroes_cnt = 0;
    for (unsigned c = 0; c < 3; c++)
      for (unsigned r = 0; r < 3; r++)
        if (fabsf(inout_tm.m[c][r]) < 1e-6)
          zeroes_cnt++;
    if (zeroes_cnt == 6)
    {
      inout_box_extents = inout_tm % inout_box_extents;
      inout_tm.setcol(0, Point3(1, 0, 0));
      inout_tm.setcol(1, Point3(0, 1, 0));
      inout_tm.setcol(2, Point3(0, 0, 1));
      return true;
    }
    return false;
  }

  static void clearAllocatedData(PhysCollision &);

protected:
  TYPE collType;
  float collMargin = 0;

  friend class PhysBody;
  friend class PhysWorld;
  static void clearNativeShapeData(PhysCollision &);
};


class PhysBoxCollision : public PhysCollision
{
public:
  PhysBoxCollision(float size_x, float size_y, float size_z, bool auto_margin = false) : PhysCollision(TYPE_BOX)
  {
    size = Point3(fabsf(size_x), fabsf(size_y), fabsf(size_z));
    if (auto_margin)
      collMargin = min(min(size.x, min(size.y, size.z)) * 0.05f, 0.04f);
  }
  Point3 getSize() const { return size; }

protected:
  Point3 size;

  friend class PhysBody;
};


class PhysSphereCollision : public PhysCollision
{
public:
  PhysSphereCollision(real rad) : PhysCollision(TYPE_SPHERE), radius(rad) { G_ASSERTF(radius > 0, "radius=%g", radius); }
  float getRadius() const { return radius; }

protected:
  real radius;

  friend class PhysBody;
};


class PhysCapsuleCollision : public PhysCollision
{
public:
  PhysCapsuleCollision(real _radius, real _height, int dir_axis_idx = 0 /*X*/) :
    PhysCollision(TYPE_CAPSULE), radius(_radius), height(_height - _radius * 2.0f), dirIdx(dir_axis_idx)
  {
    G_ASSERTF(_radius > 0, "_radius=%g", _radius);
    G_ASSERTF(_height > 0, "_height=%g", _height);
    G_ASSERT(this->height >= 0);
  }

protected:
  real radius;
  real height;
  int dirIdx;

  friend class PhysBody;
};


class PhysCylinderCollision : public PhysCollision
{
public:
  PhysCylinderCollision(real _radius, real _height, int dir_axis_idx = 0 /*X*/) :
    PhysCollision(TYPE_CYLINDER), radius(_radius), height(_height), dirIdx(dir_axis_idx)

  {
    G_ASSERTF(_radius > 0, "_radius=%g", _radius);
    G_ASSERTF(_height > 0, "_height=%g", _height);
  }

protected:
  real radius;
  real height;
  int dirIdx;

  friend class PhysBody;
};


class PhysPlaneCollision : public PhysCollision
{
  Plane3 plane;

public:
  PhysPlaneCollision(const Plane3 &plane_) : PhysCollision(TYPE_PLANE), plane(plane_) {}
  friend class PhysBody;
};


class PhysHeightfieldCollision : public PhysCollision
{
  const HeightmapHandler *hmap;
  LandMeshHolesManager *holes;
  float scale = 1.f, hmStep = 2.f;
  Point2 ofs = Point2(0, 0);
  BBox3 box;

public:
  PhysHeightfieldCollision(const HeightmapHandler *hm, const Point2 &hm_ofs, const BBox3 &bbox, float hm_scale = 1.f,
    float hm_step = 2.f, LandMeshHolesManager *lm_holes = nullptr) :
    PhysCollision(TYPE_HEIGHTFIELD), hmap(hm), ofs(hm_ofs), scale(hm_scale), box(bbox), hmStep(hm_step), holes(lm_holes)
  {}
  friend class PhysBody;
};


class PhysTriMeshCollision : public PhysCollision
{
  const void *vdata, *idata;
  Point3 scale = Point3(1, 1, 1);
  unsigned vstride, vnum, istride, inum;
  bool vtypeShort, revNorm;

public:
  template <typename VV, typename VI, typename VType = typename VV::value_type, typename IType = typename VI::value_type>
  PhysTriMeshCollision(const VV &vert, const VI &idx, const float *scl = nullptr, bool vert_short = false, bool rev_norm = false) :
    PhysCollision(TYPE_TRIMESH), vtypeShort(vert_short), revNorm(rev_norm)
  {
    vdata = vert.data();
    vstride = elem_size(vert);
    vnum = vert.size();
    idata = idx.data();
    istride = elem_size(idx);
    inum = idx.size();
    if (scl)
      scale = Point3(scl[0], scl[1], scl[2]);
  }
  friend class PhysBody;
};


class PhysConvexHullCollision : public PhysCollision
{
  const float *vdata;
  unsigned vstride, vnum;
  bool build;

public:
  PhysConvexHullCollision(const float *vert, unsigned _vnum, unsigned _vstride, bool build_hull) :
    PhysCollision(TYPE_CONVEXHULL), vdata(vert), vnum(_vnum), vstride(_vstride), build(build_hull)
  {}
  template <typename V, typename T = typename V::value_type>
  PhysConvexHullCollision(const V &vert, bool build_hull) :
    PhysConvexHullCollision((const float *)vert.data(), vert.size(), sizeof(T), build_hull)
  {}
  friend class PhysBody;
};


class PhysCompoundCollision : public PhysCollision
{
  struct Child
  {
    TMatrix m;
    PhysCollision *c;
  };
  Tab<Child> coll;

public:
  PhysCompoundCollision() : PhysCollision(TYPE_COMPOUND) {}
  PhysCompoundCollision(const PhysCompoundCollision &) = delete;
  ~PhysCompoundCollision() { clear(); }
  void addChildCollision(PhysCollision *c, TMatrix m) { coll.push_back({m, c}); }
  unsigned getChildrenCount() const { return coll.size(); }
  void clear()
  {
    for (auto c : coll)
    {
      PhysCollision::clearAllocatedData(*c.c);
      delete c.c;
    }
    clear_and_shrink(coll);
  }
  friend class PhysBody;
};

inline void PhysCollision::clearAllocatedData(PhysCollision &c)
{
  if (c.collType == TYPE_COMPOUND)
    static_cast<PhysCompoundCollision &>(c).clear();
  else if (c.collType == TYPE_NATIVE_SHAPE)
    clearNativeShapeData(c);
}
