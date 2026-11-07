// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "mesh/polygon.h"
#include "prims/tessellate.h"
#include <math/dag_check_nan.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

namespace dacsg
{
constexpr double PI_D = 3.14159265358979323846;
constexpr uint32_t SMOOTH = 1;

inline bool positive(float x) { return check_finite(x) && x > 0; }
template <typename... Args>
inline bool fail(eastl::string &err, const char *format, Args... args)
{
  err.sprintf(format, args...);
  return false;
}

// Planar uv; the added points' bbox maps to 0..1
struct Chart
{
  Point3 axisU, axisV;
  Point2 lo = Point2(1e30f, 1e30f), hi = Point2(-1e30f, -1e30f);

  explicit Chart(const Point3 &n);
  void add(const Point3 &p);
  Point2 operator()(const Point3 &p) const;
};

// count 1 is a pole
struct Ring
{
  uint32_t first = 0, count = 0;
  float v = 0;
  Point2 n = Point2(0, 0); // (radius, y) normal for the band; zero: none
};

// CH_NORMAL: a face's three corner normals, else its flat normal
struct PrimBuilder
{
  Mesh &m;
  uint32_t prim;
  dag::Vector<Point2> uv;
  dag::Vector<Point3> nrm;        // per corner; zero: the vertex's on a smooth face
  dag::Vector<Point3> vertNormal; // per vertex, or empty

  uint32_t vert(const Point3 &p, const Point3 &n = Point3(0, 0, 0));
  void tri(uint32_t a, uint32_t b, uint32_t c, Point2 ta, Point2 tb, Point2 tc, uint32_t smgr, const Point3 (&n)[3] = {});
  // CCW from outside; keeps convex quads convex
  void quad(uint32_t a, uint32_t b, uint32_t c, uint32_t d, Point2 ta, Point2 tb, Point2 tc, Point2 td, uint32_t smgr,
    const Point3 (&n)[4] = {});
  // Ring a has the lower v; dirs: the lathe's columns, for the rings' normals
  void band(const Ring &a, const Ring &b, uint32_t segs, uint32_t smgr, const Chart *chart = nullptr,
    dag::ConstSpan<Point2> dirs = {});
  // A flat polygon of the vertices vtx(0..count - 1), tris indexing them, with a planar chart; flip turns each triangle
  template <class Vtx>
  void cap(const Point3 &normal, uint32_t count, dag::ConstSpan<uint32_t> tris, bool flip, Vtx vtx)
  {
    Chart chart(normal);
    for (uint32_t k = 0; k < count; ++k)
      chart.add(m.pos[vtx(k)]);
    for (uint32_t i = 0; i < tris.size(); i += 3)
    {
      const uint32_t v0 = vtx(tris[i]), v1 = vtx(tris[i + (flip ? 2 : 1)]), v2 = vtx(tris[i + (flip ? 1 : 2)]);
      tri(v0, v1, v2, chart(m.pos[v0]), chart(m.pos[v1]), chart(m.pos[v2]), 0);
    }
  }
  void finish();
};

struct ProfilePoint
{
  Point2 p; // (radius, y)
  float v = 0;
  uint32_t smgr = 0;       // of the band to the next point
  bool cap = false;        // that band is flat: planar uv, smgr 0
  Point2 n = Point2(0, 0); // the exact normal, or zero: from the segments
};

// Outward for a CCW profile or polygon
inline Point2 segment_normal(const Point2 &a, const Point2 &b) { return normalize(Point2(b.y - a.y, a.x - b.x)); }

// Exact at multiples of 90 deg
Point2 unit_dir(double deg);
// A CCW (radius, y) profile faces outward
void lathe(PrimBuilder &b, dag::ConstSpan<ProfilePoint> prof, bool loop, uint32_t segs, double angle,
  dag::Vector<Ring> *rings_out = nullptr);

struct Lattice
{
  dag::Vector<double> t[3];         // ascending samples on the cube surface
  DPoint3 inner = DPoint3(0, 0, 0); // points move to clamp(p) + r * dir
  double r = 0;
  bool uvByIndex = false;
};
// Cells within the inner box are flat
void cube_lattice(PrimBuilder &b, const Lattice &l);

struct PrimArgs
{
  const Graph &g;
  const PrimDesc &d;
  uint32_t seg = 0, segMinor = 0;
  MeshLoader loader = nullptr;
  bool checkOnly = false; // an import checks that it has a loader and reads no file
};

using PrimFn = bool (*)(const PrimArgs &, PrimBuilder &, eastl::string &);
bool make_box(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_rbox(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_sphere(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_ellipsoid(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_icosphere(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_cubesphere(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_cylinder(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_cone(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_ccyl(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_capsule(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_torus(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_convex(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_extrude(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_revolve(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_tube(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_loft(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
bool make_mesh(const PrimArgs &a, PrimBuilder &b, eastl::string &err);
} // namespace dacsg
