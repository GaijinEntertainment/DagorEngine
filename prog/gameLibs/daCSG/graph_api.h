// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <debug/dag_assert.h>
#include <generic/dag_functionRef.h>
#include <generic/dag_span.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <util/dag_hashChain.h>
#include <util/dag_stdint.h>
#include <EASTL/string.h>

namespace dacsg
{
struct ILeafSource;
struct Mesh;

// The reader of Graph::mesh, ExecuteParams::meshLoader: the client picks the formats; the path comes as the model gave it; it
// overwrites out. Several imports load at once on the execute's workers, so the loader must be reentrant;
// it does not own the callable, which must outlive the execute; a plain function binds only in a lambda
using MeshLoader = dag::FunctionRef<bool(const char *path, Mesh &out, eastl::string &err) const>;

struct PlaneD
{
  DPoint3 n;
  double d;
};

struct TMatrixD
{
  DPoint3 col[4];
};

enum class NodeKind : uint8_t
{
  Prim,
  Union,
  Subtract,
  Intersect,
  Concat,
  Cut,
  Transform,
  Attr,
  Empty,
};

struct OpResult
{
  uint32_t id = 0;
};
inline bool operator==(OpResult a, OpResult b) { return a.id == b.id; }
inline bool operator!=(OpResult a, OpResult b) { return a.id != b.id; }

enum class PrimType : uint16_t
{
  Box,
  RBox,
  Sphere,
  Ellipsoid,
  IcoSphere,
  CubeSphere,
  Cylinder,
  Cone,
  CCyl,
  Capsule,
  Torus,
  Convex,
  Extrude,
  Revolve,
  Tube,
  Loft,
  Leaf,
  Mesh,
};

struct PointRange
{
  uint32_t first = 0, count = 0;
};

struct PrimDesc
{
  enum Param : uint8_t
  {
    SIZE_X = 0,
    SIZE_Y = 1,
    SIZE_Z = 2,
    RBOX_RADIUS = 3,
    RADIUS = 0,
    RADIUS_X = 0,
    RADIUS_Y = 1,
    RADIUS_Z = 2,
    HEIGHT = 1,
    CONE_RADIUS_TOP = 1,
    CONE_HEIGHT = 2,
    CCYL_CHAMFER = 2,
    TORUS_RADIUS_MINOR = 1,
    EXTRUDE_HEIGHT = 0,
    LOFT_SECTION_POINTS = 0,
    LOFT_CLOSED = 1, // 1: the last section joins the first
    PARAM_COUNT = 4,
  };

  PrimType type = PrimType::Box;
  uint16_t level = 0;
  uint16_t seg = 0;      // 0: from the execute's sag, spec.md 4
  uint16_t segMinor = 0; // 0: seg / 2 when seg is set, else from the sag
  uint32_t path = 0;     // mesh: the path index; leaf: the source index
  float p[PARAM_COUNT] = {};
  float angleDeg = 360.f;
  PointRange points; // convex, tube, loft: points3; extrude, revolve: points2
};

enum class AttrKind : uint8_t
{
  Color,
  Mat,
  Flat,
  Smooth,
  UvPlanar,
  UvBox,
  UvCyl,
  UvSphere,
  UvAffine,
};

struct AttrDesc
{
  AttrKind kind = AttrKind::Color;
  bool onlyUnset = false;                        // every kind but Flat and Smooth: skip a face whose attribute a user set
  uint16_t mat = 0;                              // Mat
  float color[4] = {};                           // Color
  Point3 axis = Point3(0, 1, 0);                 // UvPlanar, UvCyl, UvSphere
  float scale = 1.f;                             // UvPlanar, UvBox, UvCyl; UvSphere takes none
  float uvAffine[2][3] = {{1, 0, 0}, {0, 1, 0}}; // UvAffine: rows map (u, v, 1) to u, v
};

struct Node
{
  NodeKind kind = NodeKind::Empty;
  OpResult a, b;
  uint32_t payload = 0;
};

// Hash-consed by kind, children, payload; not name.
class Graph
{
public:
  Graph();

  // any type but PrimType::Leaf, which only leaf() makes; a refused type asserts and returns empty()
  OpResult prim(const PrimDesc &d);
  OpResult box(const Point3 &size);
  OpResult rbox(const Point3 &size, float radius, int seg = 0);
  OpResult sphere(float r, int seg = 0);
  OpResult ellipsoid(const Point3 &radii, int seg = 0);
  OpResult icosphere(float r, int level = 3);
  OpResult cubesphere(float r, int seg = 0);
  OpResult cylinder(float r, float h, int seg = 0);
  OpResult cone(float r_bottom, float r_top, float h, int seg = 0);
  OpResult ccyl(float r, float h, float chamfer, int seg = 0);
  OpResult capsule(float r, float h, int seg = 0);
  OpResult torus(float r_major, float r_minor, int seg = 0, int seg_minor = 0);
  OpResult convex(dag::ConstSpan<Point3> points);
  OpResult extrude(dag::ConstSpan<Point2> poly, float h);
  OpResult revolve(dag::ConstSpan<Point2> profile, float angle_deg = 360.f, int seg = 0);
  OpResult tube(dag::ConstSpan<Point3> path, float r, int seg = 0);
  // Closed sections of section_points each, one after another
  OpResult loft(dag::ConstSpan<Point3> sections, uint32_t section_points, bool closed = false);
  OpResult mesh(const char *path);
  // One node per content: contentHash, then equals
  OpResult leaf(ILeafSource *source);
  OpResult empty() const { return OpResult(); }

  // Union, Subtract, Intersect or Concat: the other kinds have their own builders
  OpResult op(NodeKind kind, OpResult a, OpResult b);
  // cut, transform and attr check their values at execute; spec.md 11 gives each failure text
  OpResult cut(OpResult a, const PlaneD &keep_below);
  OpResult transform(OpResult a, const TMatrixD &tm);
  OpResult attr(OpResult a, const AttrDesc &d);

  // generated: a display name copied from another graph's node, which ownName does not return
  void setName(OpResult r, const char *name, bool generated = false);
  // The call that made the node: the first one wins
  void setSource(OpResult r, const char *file, uint32_t line);
  eastl::string getName(OpResult r) const;                          // else <type>#<id>, then (file:line)
  const char *ownName(OpResult r) const;                            // nullptr without setName
  eastl::string autoName(OpResult r) const;                         // <type>#<id> and source, ignoring setName
  bool source(OpResult r, const char *&file, uint32_t &line) const; // false without setSource
  float getQuality() const { return quality; }
  void setQuality(float q) { quality = q; }

  uint32_t nodeCount() const { return nodes.size(); }
  const Node &node(OpResult r) const { return nodes[r.id]; }
  const PrimDesc &primDesc(OpResult r) const
  {
    G_ASSERT(node(r).kind == NodeKind::Prim);
    return prims[node(r).payload];
  }
  const PlaneD &plane(OpResult r) const
  {
    G_ASSERT(node(r).kind == NodeKind::Cut);
    return planes[node(r).payload];
  }
  const TMatrixD &tm(OpResult r) const
  {
    G_ASSERT(node(r).kind == NodeKind::Transform);
    return tms[node(r).payload];
  }
  const AttrDesc &attrDesc(OpResult r) const
  {
    G_ASSERT(node(r).kind == NodeKind::Attr);
    return attrs[node(r).payload];
  }

  PointRange addPoints3(dag::ConstSpan<Point3> pts);
  PointRange addPoints2(dag::ConstSpan<Point2> pts);
  uint32_t addPath(const char *path);
  dag::ConstSpan<Point3> points3(PointRange r) const { return dag::ConstSpan<Point3>(pts3.data() + r.first, r.count); }
  dag::ConstSpan<Point2> points2(PointRange r) const { return dag::ConstSpan<Point2>(pts2.data() + r.first, r.count); }
  const char *path(uint32_t i) const { return paths[i].c_str(); }
  ILeafSource *leafSource(uint32_t i) const { return leafSources[i]; }

private:
  struct Source
  {
    uint32_t file = 0, line = 0; // line 0: unknown
  };

  template <class T>
  OpResult intern(Node n, dag::Vector<T> *store, const T *payload);
  eastl::string withSource(eastl::string text, OpResult r) const;

  dag::Vector<Node> nodes;
  dag::Vector<PrimDesc> prims;
  dag::Vector<PlaneD> planes;
  dag::Vector<TMatrixD> tms;
  dag::Vector<AttrDesc> attrs;
  dag::Vector<Point3> pts3;
  dag::Vector<Point2> pts2;
  dag::Vector<eastl::string> paths;
  dag::Vector<ILeafSource *> leafSources;
  HashChain leafByHash;
  dag::Vector<eastl::string> names;
  dag::Vector<uint8_t> generatedName; // with names
  dag::Vector<Source> sources;
  dag::Vector<eastl::string> sourceFiles;
  HashChain nodeByHash;
  dag::Vector<PointRange> pts3Ranges, pts2Ranges;
  HashChain pts3ByHash, pts2ByHash;
  float quality = 1.f;
};

const char *prim_type_name(PrimType t);
const char *node_kind_name(NodeKind k);
} // namespace dacsg
