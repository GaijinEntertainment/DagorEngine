//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_bounds3.h>
#include <math/dag_Point3.h>
#include <util/dag_globDef.h>

/// does 4x4x4 FFD box transformation. Currently does in world space
///  (latice space is world axis aligned box), but can be easily modified
///  to deform in any other space.
//
class FFDDeformer44
{
protected:
  static constexpr int PT_COUNT = 64;
  Point3 pt[PT_COUNT];
  BBox3 latticeBox;
  static inline int gridIndex(int i, int j, int k) { return (((i) << 4) + ((j) << 2) + (k)); }
  static inline float BPoly4(int i, float u)
  {
    float s = 1.0f - u;
    switch (i)
    {
      case 0: return s * s * s;
      case 1: return 3.0f * u * s * s;
      case 2: return 3.0f * u * u * s;
      case 3: return u * u * u;
      default: return 0.0f;
    }
  }

public:
  FFDDeformer44();
  /// set lattice space (currently, world-aligned)
  inline void setBox(const BBox3 &box) { latticeBox = box; }
  /// @return control point
  inline const Point3 &getPoint(int i, int j, int k) const
  {
    G_ASSERT(i >= 0 && i < 4 && j >= 0 && j < 4 && k >= 0 && k < 4);
    return pt[gridIndex(i, j, k)];
  }
  inline const Point3 &getPointByIdx(unsigned idx) const
  {
    G_ASSERT(idx < PT_COUNT);
    return pt[idx];
  }

  /// modify control point
  inline void setPoint(int i, int j, int k, const Point3 &p)
  {
    G_ASSERT(i >= 0 && i < 4 && j >= 0 && j < 4 && k >= 0 && k < 4);
    pt[gridIndex(i, j, k)] = p;
  }
  inline void setPointByIdx(unsigned idx, const Point3 &p)
  {
    G_ASSERT(idx < PT_COUNT);
    pt[idx] = p;
  }

  static const Point3 getDefaultPoint(int i, int j, int k) { return Point3(float(i) / 3.0f, float(j) / 3.0f, float(k) / 3.0f); }
  static const Point3 getDefaultPoint(int i)
  {
    return Point3(float((i >> 4) & 3) / 3.0f, float((i >> 2) & 3) / 3.0f, float(i & 3) / 3.0f);
  }

  // reset control points
  void noDeform();
  // deforms all except corner (and edge points, if @preserve_only_corners is false)
  void crashDeform(float scale, bool preserve_only_corners);

  // the deformation itself
  Point3 deform(const Point3 &p, bool in_bound_only = false) const;

  Point3 transformTo(const Point3 &p) const
  {
    // pp = tm*p;
    Point3 pp = (p - latticeBox[0]);
    pp.x /= (latticeBox.width().x);
    pp.y /= (latticeBox.width().y);
    pp.z /= (latticeBox.width().z);
    return pp;
  }
  Point3 transformFrom(const Point3 &p) const
  {
    // pp = itm*p;
    Point3 pp = p;
    pp.x *= (latticeBox.width().x);
    pp.y *= (latticeBox.width().y);
    pp.z *= (latticeBox.width().z);
    return pp + latticeBox[0];
  }
};
/*
  //how to deform car: moved from dynsceneressrc.cpp, Line 180
  #include <util/dag_stdint.h>
  int64_t refTime = ref_time_ticks ();

  FFDDeformer44 deformer;
  deformer.crashDeform(0.25, false);//strength of deform

  //bounding box of the car (in world units, offseted to car origin)
  deformer.setBox(BBox3(Point3(-1.07,-0.5,-2.3), Point3(1.07,0.6,2.3)));
  TMatrix invWtm = inverse(key_node->wtm);
  float time = (float)perlin_noise::Perm(0); //time
  for (i=0; i<mesh.vert.size(); ++i)
  {
    Point3 vertWorld = key_node->wtm * mesh.vert[i];//transform to world space, near origin
    vertWorld = deformer.deform(vertWorld); //deform with FFD
    //add 3d fractal noise with hardcoded params.
    vertWorld += 0.25f * perlin_noise::fractal_noise(
        vertWorld,
        1/4.0f,//scale = 4
        1-0.25f,//rough = 0.25
        6,//iterations
        time
      );
    //transform back again
    mesh.vert[i] = invWtm * vertWorld;
  }
  //takes up to 16 000 us (60 fps) for whole car.
  //This can be speed up (if double transformation is not needed for vertices, common case)
  //  or reduced (if only a part of car)
  debug("mesh %s %d us", (const char*)lod.fileName, get_time_usec ( refTime ));
*/
